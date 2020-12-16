/*
   Copyright 2020 The Silkrpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <chrono>
#include <coroutine>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <system_error>
#include <thread>
#include <variant>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/coro/task.hpp>
#include <silkrpc/kv/remote/kv.grpc.pb.h>

ABSL_FLAG(std::string, table, "", "database table name");
ABSL_FLAG(std::string, seekkey, "", "seek key as hex string w/o leading 0x");
ABSL_FLAG(std::string, target, silkrpc::kv::kDefaultTarget, "server location as string <address>:<port>");
ABSL_FLAG(uint32_t, timeout, silkrpc::kv::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");

using namespace silkworm;

class GrpcKvCallbackReactor final : public grpc::experimental::ClientBidiReactor<remote::Cursor, remote::Pair> {
public:
    explicit GrpcKvCallbackReactor(remote::KV::Stub& stub) : stub_(stub) {
        stub_.experimental_async()->Tx(&context_, this);
        StartCall();
    }

    void read_start(std::function<void(bool,remote::Pair)> read_completed) {
        read_completed_ = read_completed;
        StartRead(&pair_);
    }

    void write_start(remote::Cursor* cursor, std::function<void(bool)> write_completed) {
        write_completed_ = write_completed;
        StartWrite(cursor);
    }

    void OnReadDone(bool ok) override {
        read_completed_(ok, pair_);
    }

    void OnWriteDone(bool ok) override {
        write_completed_(ok);
    }
private:
    remote::KV::Stub& stub_;
    grpc::ClientContext context_;
    remote::Pair pair_;
    std::function<void(bool,remote::Pair)> read_completed_;
    std::function<void(bool)> write_completed_;
};

struct KvAwaitable {
    KvAwaitable(asio::io_context& context, GrpcKvCallbackReactor& reactor, uint32_t cursor_id = 0)
    : context_(context), reactor_(reactor), cursor_id_(cursor_id) {}

    bool await_ready() { return false; }

protected:
    asio::io_context& context_;
    GrpcKvCallbackReactor& reactor_;
    uint32_t cursor_id_;
};

struct KvOpenCursorAwaitable : KvAwaitable {
    KvOpenCursorAwaitable(asio::io_context& context, GrpcKvCallbackReactor& reactor, const std::string& table_name)
    : KvAwaitable{context, reactor}, table_name_(table_name) {}

    auto await_suspend(std::coroutine_handle<> handle) {
        auto open_message = remote::Cursor{};
        open_message.set_op(remote::Op::OPEN);
        open_message.set_bucketname(table_name_);
        reactor_.write_start(&open_message, [&, handle](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in OPEN cursor"};
            }
            std::cout << "Write callback thread: " << std::this_thread::get_id() << "\n" << std::flush;
            std::cout << "KV Tx OPEN -> table_name: " << table_name_ << "\n";
            reactor_.read_start([&, handle](bool ok, remote::Pair open_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in OPEN cursor"};
                }
                std::cout << "Read callback thread: " << std::this_thread::get_id() << "\n" << std::flush;
                std::cout << "KV Tx OPEN <- cursor: " << open_pair.cursorid() << "\n";
                cursor_id_ = open_pair.cursorid();
                //handle(); // TODO: replace with enqueue coro in coop_scheduler (io_context)
                context_.post([&handle]() { handle.resume(); });
            });
        });
    }

    auto await_resume() noexcept { return cursor_id_; }

private:
    const std::string& table_name_;
};

struct KvSeekAwaitable : KvAwaitable {
    KvSeekAwaitable(asio::io_context& context, GrpcKvCallbackReactor& reactor, uint32_t cursor_id, const silkworm::Bytes& seek_key_bytes)
    : KvAwaitable{context, reactor, cursor_id}, seek_key_bytes_(std::move(seek_key_bytes)) {}

    auto await_suspend(std::coroutine_handle<> handle) {
        auto seek_message = remote::Cursor{};
        seek_message.set_op(remote::Op::SEEK);
        seek_message.set_cursor(cursor_id_);
        seek_message.set_k(seek_key_bytes_.c_str(), seek_key_bytes_.length());
        reactor_.write_start(&seek_message, [&, handle](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in SEEK"};
            }
            std::cout << "Write callback thread: " << std::this_thread::get_id() << "\n" << std::flush;
            std::cout << "KV Tx SEEK -> cursor: " << cursor_id_ << " seek_key: " << seek_key_bytes_ << "\n";
            reactor_.read_start([&, handle](bool ok, remote::Pair seek_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in SEEK"};
                }
                std::cout << "Read callback thread: " << std::this_thread::get_id() << "\n" << std::flush;
                const auto& key_bytes = silkworm::byte_view_of_string(seek_pair.k());
                const auto& value_bytes = silkworm::byte_view_of_string(seek_pair.v());
                std::cout << "KV Tx SEEK <- key: " << key_bytes << " value: " << value_bytes << std::endl;
                value_bytes_ = std::move(value_bytes);
                //handle(); // TODO: replace with enqueue coro in coop_scheduler (io_context)
                context_.post([&handle]() { handle.resume(); });
            });
        });
    }

    auto await_resume() noexcept { return value_bytes_; }

private:
    const silkworm::Bytes seek_key_bytes_;
    silkworm::ByteView value_bytes_;
};

struct KvCloseCursorAwaitable : KvAwaitable {
    KvCloseCursorAwaitable(asio::io_context& context, GrpcKvCallbackReactor& reactor, uint32_t cursor_id)
    : KvAwaitable{context, reactor, cursor_id} {}

    auto await_suspend(std::coroutine_handle<> handle) {
        auto close_message = remote::Cursor{};
        close_message.set_op(remote::Op::CLOSE);
        close_message.set_cursor(cursor_id_);
        reactor_.write_start(&close_message, [&, handle](bool ok) {
            if (!ok) {
                throw std::system_error{std::make_error_code(std::errc::io_error), "write failed in CLOSE cursor"};
            }
            std::cout << "Write callback thread: " << std::this_thread::get_id() << "\n" << std::flush;
            std::cout << "KV Tx CLOSE -> cursor_id: " << cursor_id_ << "\n";
            reactor_.read_start([&, handle](bool ok, remote::Pair close_pair) {
                if (!ok) {
                    throw std::system_error{std::make_error_code(std::errc::io_error), "read failed in CLOSE cursor"};
                }
                std::cout << "Read callback thread: " << std::this_thread::get_id() << "\n" << std::flush;
                std::cout << "KV Tx CLOSE <- cursor: " << close_pair.cursorid() << "\n";
                cursor_id_ = close_pair.cursorid();
                //handle(); // TODO: replace with enqueue coro in coop_scheduler (io_context)
                context_.post([&handle]() { handle.resume(); });
            });
        });
    }

    auto await_resume() noexcept { return cursor_id_; }
};

class KvService {
public:
    explicit KvService(asio::io_context& context, remote::KV::Stub& stub)
    : context_(context), reactor_{stub} {}

    task<uint32_t> open_cursor(const std::string& table_name) {
        auto cursor_id = co_await KvOpenCursorAwaitable{context_, reactor_, table_name};
        co_return cursor_id;
        // co_return KvOpenCursorAwaitable{reactor_, table_name}; does it work this way?
    }

    task<silkworm::ByteView> seek(uint32_t cursor_id, const silkworm::Bytes& seek_key) {
        auto value_bytes = co_await KvSeekAwaitable{context_, reactor_, cursor_id, seek_key};
        co_return value_bytes;
        // co_return KvSeekAwaitable{reactor_, cursor_id, seek_key}; does it work this way?
    }

    task<void> close_cursor(uint32_t cursor_id) {
        co_await KvCloseCursorAwaitable{context_, reactor_, cursor_id};
        co_return; // does it work without this?
    }

private:
    asio::io_context& context_;
    GrpcKvCallbackReactor reactor_;
};

task<void> kv_seek(KvService& kv_svc, const std::string& table_name, const silkworm::Bytes& seek_key) {
    std::cout << "kv_seek STEP1 thread: " << std::this_thread::get_id() << "\n" << std::flush;
    auto cursor_id = co_await kv_svc.open_cursor(table_name);
    std::cout << "kv_seek STEP2 thread: " << std::this_thread::get_id() << "\n" << std::flush;
    auto value = co_await kv_svc.seek(cursor_id, seek_key);
    std::cout << "kv_seek STEP3 thread: " << std::this_thread::get_id() << "\n" << std::flush;
    co_await kv_svc.close_cursor(cursor_id);
    std::cout << "kv_seek STEP4 thread: " << std::this_thread::get_id() << "\n" << std::flush;
    co_return;
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Seek Turbo-Geth/Silkworm Key-Value (KV) remote interface to database");
    absl::ParseCommandLine(argc, argv);

    std::cout << "Main thread: " << std::this_thread::get_id() << "\n" << std::flush;

    using namespace std::chrono;
    using namespace silkworm;

    auto table_name{absl::GetFlag(FLAGS_table)};
    if (table_name.empty()) {
        std::cerr << "Parameter table is invalid: [" << table_name << "]\n";
        std::cerr << "Use --table flag to specify the name of Turbo-Geth database table\n";
        return -1;
    }

    auto seek_key{absl::GetFlag(FLAGS_seekkey)};
    if (seek_key.empty() || false /*is not hex*/) {
        std::cerr << "Parameter seek key is invalid: [" << seek_key << "]\n";
        std::cerr << "Use --seekkey flag to specify the seek key in Turbo-Geth database table\n";
        return -1;
    }

    auto target{absl::GetFlag(FLAGS_target)};
    if (target.empty() || target.find(":") == std::string::npos) {
        std::cerr << "Parameter target is invalid: [" << target << "]\n";
        std::cerr << "Use --target flag to specify the location of Turbo-Geth running instance\n";
        return -1;
    }

    auto timeout{absl::GetFlag(FLAGS_timeout)};
    if (timeout < 0) {
        std::cerr << "Parameter timeout is invalid: [" << timeout << "]\n";
        std::cerr << "Use --timeout flag to specify the timeout in msecs for Turbo-Geth KV gRPC calls\n";
        return -1;
    }

    asio::io_context context;
    //asio::io_context::work work{context};

    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto stub = remote::KV::NewStub(channel);

    asio::signal_set signals(context, SIGINT, SIGTERM);
    signals.async_wait([&](const asio::system_error& error, int signal_number) {
        std::cout << "Signal caught, error: " << error.what() << " number: " << signal_number << std::endl << std::flush;
        std::cout << "Signal thread: " << std::this_thread::get_id() << std::endl << std::flush;
        context.stop();
    });

    const auto& seek_key_bytes = from_hex(seek_key);

    KvService kv_svc{context, *stub};

    task<void> result = kv_seek(kv_svc, table_name, seek_key_bytes);
    std::cout << "main task ptr: " << &result  << " result ch" << result.coroutine_handle().address() << std::endl << std::flush;

    auto handle = result.coroutine_handle();
    context.post([&handle]() { handle.resume(); });

    context.run();

    return 0;
}
