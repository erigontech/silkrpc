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

//#include <coroutine>
#include <functional>
#include <iomanip>
#include <iostream>
#include <utility>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#define ASIO_HAS_CO_AWAIT
#define ASIO_HAS_STD_COROUTINE
#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/coro/coroutine.hpp>
//#include <silkrpc/coro/task.hpp>
#include <silkrpc/kv/awaitables.hpp>
#include <silkrpc/kv/client_callback_reactor.hpp>
#include <silkrpc/kv/remote_client.hpp>
#include <silkrpc/kv/remote/kv.grpc.pb.h>

ABSL_FLAG(std::string, table, "", "database table name");
ABSL_FLAG(std::string, seekkey, "", "seek key as hex string w/o leading 0x");
ABSL_FLAG(std::string, target, silkrpc::kv::kDefaultTarget, "server location as string <address>:<port>");
ABSL_FLAG(uint32_t, timeout, silkrpc::kv::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");

using namespace silkworm;
using namespace silkrpc::coro;
using namespace silkrpc::kv;

asio::awaitable<void> kv_seek(asio::io_context& context, RemoteClient& kv_client, const std::string& table_name, const silkworm::Bytes& seek_key) {
    std::cout << "KV Tx OPEN -> table_name: " << table_name << "\n" << std::flush;
    auto cursor_id = co_await kv_client.open_cursor(table_name);
    std::cout << "KV Tx OPEN <- cursor: " << cursor_id << "\n" << std::flush;
    std::cout << "KV Tx SEEK -> cursor: " << cursor_id << " seek_key: " << seek_key << "\n" << std::flush;
    auto kv_pair = co_await kv_client.seek(cursor_id, seek_key);
    std::cout << "KV Tx SEEK <- key: " << kv_pair.key << " value: " << kv_pair.value << "\n" << std::flush;
    std::cout << "KV Tx CLOSE -> cursor: " << cursor_id << "\n" << std::flush;
    co_await kv_client.close_cursor(cursor_id);
    std::cout << "KV Tx CLOSE <- cursor: 0\n" << std::flush;
    co_return;
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Seek Turbo-Geth/Silkworm Key-Value (KV) remote interface to database");
    absl::ParseCommandLine(argc, argv);

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
    asio::io_context::work work{context};

    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());

    RemoteClient kv_client{context, channel};

    asio::co_spawn(context, kv_seek(context, kv_client, table_name, from_hex(seek_key)), [&](std::exception_ptr exptr) {
        context.stop();
    });

    context.run();

    return 0;
}
