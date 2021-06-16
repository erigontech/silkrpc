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

#include <silkrpc/config.hpp>

#include <functional>
#include <iomanip>
#include <iostream>
#include <utility>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <asio/co_spawn.hpp>
#include <grpcpp/grpcpp.h>
#include <silkworm/common/util.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>

ABSL_FLAG(std::string, table, "", "database table name");
ABSL_FLAG(std::string, seekkey, "", "seek key as hex string w/o leading 0x");
ABSL_FLAG(std::string, target, silkrpc::common::kDefaultTarget, "server location as string <address>:<port>");
ABSL_FLAG(uint32_t, timeout, silkrpc::common::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");
ABSL_FLAG(silkrpc::LogLevel, logLevel, silkrpc::LogLevel::Critical, "logging level");

using silkrpc::LogLevel;

asio::awaitable<void> kv_seek(silkrpc::ethdb::Database& kv_db, const std::string& table_name, const silkworm::Bytes& seek_key) {
    const auto kv_transaction = co_await kv_db.begin();
    std::cout << "KV Tx OPEN -> table_name: " << table_name << "\n" << std::flush;
    const auto kv_cursor = co_await kv_transaction->cursor(table_name);
    auto cursor_id = kv_cursor->cursor_id();
    std::cout << "KV Tx OPEN <- cursor: " << cursor_id << "\n" << std::flush;
    std::cout << "KV Tx SEEK -> cursor: " << cursor_id << " seek_key: " << seek_key << "\n" << std::flush;
    auto kv_pair = co_await kv_cursor->seek(seek_key);
    std::cout << "KV Tx SEEK <- key: " << kv_pair.key << " value: " << kv_pair.value << "\n" << std::flush;
    std::cout << "KV Tx CLOSE -> cursor: " << cursor_id << "\n" << std::flush;
    co_await kv_transaction->close();
    std::cout << "KV Tx CLOSE <- cursor: 0\n" << std::flush;
    co_return;
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Seek Turbo-Geth/Silkworm Key-Value (KV) remote interface to database");
    absl::ParseCommandLine(argc, argv);

    SILKRPC_LOG_VERBOSITY(absl::GetFlag(FLAGS_logLevel));

    try {
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

        // TODO(canepat): handle also secure channel for remote
        silkrpc::ChannelFactory create_channel = [&]() {
            return grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        };
        // TODO(canepat): handle also local (shared-memory) database
        silkrpc::ContextPool context_pool{1, create_channel};
        auto& context = context_pool.get_context();
        auto& io_context = context.io_context;
        auto& database = context.database;

        asio::co_spawn(*io_context, kv_seek(*database, table_name, seek_key_bytes), [&](std::exception_ptr exptr) {
            context_pool.stop();
        });

        context_pool.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n" << std::flush;
    } catch (...) {
        std::cerr << "Unexpected exception\n" << std::flush;
    }

    return 0;
}
