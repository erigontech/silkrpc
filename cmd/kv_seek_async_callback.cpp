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
#include <functional>
#include <iomanip>
#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>

ABSL_FLAG(std::string, table, "", "database table name");
ABSL_FLAG(std::string, seekkey, "", "seek key as hex string w/o leading 0x");
ABSL_FLAG(std::string, target, silkrpc::kDefaultTarget, "server location as string <address>:<port>");
ABSL_FLAG(uint32_t, timeout, silkrpc::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");

class GrpcKvCallbackReactor final : public grpc::experimental::ClientBidiReactor<remote::Cursor, remote::Pair> {
public:
    explicit GrpcKvCallbackReactor(remote::KV::Stub& stub, std::chrono::milliseconds timeout) : stub_(stub) {
        context_.set_deadline(std::chrono::system_clock::now() + timeout);
        stub_.experimental_async()->Tx(&context_, this);
        StartCall();
    }

    void read_start(std::function<void(bool, remote::Pair)> read_completed) {
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
    std::function<void(bool, remote::Pair)> read_completed_;
    std::function<void(bool)> write_completed_;
};

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Seek Turbo-Geth/Silkworm Key-Value (KV) remote interface to database");
    absl::ParseCommandLine(argc, argv);

    auto table_name{absl::GetFlag(FLAGS_table)};
    if (table_name.empty()) {
        std::cerr << "Parameter table is invalid: [" << table_name << "]\n";
        std::cerr << "Use --table flag to specify the name of Turbo-Geth database table\n";
        return -1;
    }

    auto seek_key{absl::GetFlag(FLAGS_seekkey)};
    const auto seek_key_bytes_optional = silkworm::from_hex(seek_key);
    if (seek_key.empty() || !seek_key_bytes_optional.has_value()) {
        std::cerr << "Parameter seek key is invalid: [" << seek_key << "]\n";
        std::cerr << "Use --seekkey flag to specify the seek key in Turbo-Geth database table\n";
        return -1;
    }
    const auto seek_key_bytes = seek_key_bytes_optional.value();

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
    asio::io_context::work work{context};

    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto stub = remote::KV::NewStub(channel);

    asio::signal_set signals(context, SIGINT, SIGTERM);
    signals.async_wait([&](const asio::system_error& error, int signal_number) {
        std::cout << "Signal caught, error: " << error.what() << " number: " << signal_number << std::endl << std::flush;
        context.stop();
    });

    GrpcKvCallbackReactor reactor{*stub, std::chrono::milliseconds{timeout}};

    auto open_message = remote::Cursor{};
    open_message.set_op(remote::Op::OPEN);
    open_message.set_bucketname(table_name);
    reactor.write_start(&open_message, [&](bool ok) {
        if (!ok) {
            std::cout << "error writing OPEN gRPC" << std::flush;
            return;
        }
        std::cout << "KV Tx OPEN -> table_name: " << table_name << "\n";
        reactor.read_start([&](bool ok, remote::Pair open_pair) {
            if (!ok) {
                std::cout << "error reading OPEN gRPC" << std::flush;
                return;
            }
            const auto cursor_id = open_pair.cursorid();
            std::cout << "KV Tx OPEN <- cursor: " << cursor_id << "\n";
            auto seek_message = remote::Cursor{};
            seek_message.set_op(remote::Op::SEEK);
            seek_message.set_cursor(cursor_id);
            seek_message.set_k(seek_key_bytes.c_str(), seek_key_bytes.length());
            reactor.write_start(&seek_message, [&, cursor_id](bool ok) {
                if (!ok) {
                    std::cout << "error writing SEEK gRPC" << std::flush;
                    return;
                }
                std::cout << "KV Tx SEEK -> cursor: " << cursor_id << " seek_key: " << seek_key_bytes << "\n";
                reactor.read_start([&, cursor_id](bool ok, remote::Pair seek_pair) {
                    if (!ok) {
                        std::cout << "error reading SEEK gRPC" << std::flush;
                        return;
                    }
                    const auto& key_bytes = silkworm::byte_view_of_string(seek_pair.k());
                    const auto& value_bytes = silkworm::byte_view_of_string(seek_pair.v());
                    std::cout << "KV Tx SEEK <- key: " << key_bytes << " value: " << value_bytes << std::endl;
                    auto close_message = remote::Cursor{};
                    close_message.set_op(remote::Op::CLOSE);
                    close_message.set_cursor(cursor_id);
                    reactor.write_start(&close_message, [&, cursor_id](bool ok) {
                        if (!ok) {
                            std::cout << "error writing CLOSE gRPC" << std::flush;
                            return;
                        }
                        std::cout << "KV Tx CLOSE -> cursor: " << cursor_id << "\n";
                        reactor.read_start([&](bool ok, remote::Pair close_pair) {
                            if (!ok) {
                                std::cout << "error reading CLOSE gRPC" << std::flush;
                                return;
                            }
                            std::cout << "KV Tx CLOSE <- cursor: " << close_pair.cursorid() << "\n";
                            context.stop();
                        });
                    });
                });
            });
        });
    });

    context.run();

    return 0;
}
