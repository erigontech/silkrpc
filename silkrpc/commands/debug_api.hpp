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

#ifndef SILKRPC_COMMANDS_DEBUG_API_HPP_
#define SILKRPC_COMMANDS_DEBUG_API_HPP_

#include <memory>

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/ethdb/database.hpp>

namespace silkrpc::http { class RequestHandler; }

namespace silkrpc::commands {

class DebugRpcApi {
public:
    explicit DebugRpcApi(asio::io_context& io_context, std::unique_ptr<ethdb::Database>& database) :
        io_context_(io_context), database_(database) {}
    virtual ~DebugRpcApi() {}

    DebugRpcApi(const DebugRpcApi&) = delete;
    DebugRpcApi& operator=(const DebugRpcApi&) = delete;

protected:
    asio::awaitable<void> handle_debug_account_range(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_debug_get_modified_accounts_by_number(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_debug_get_modified_accounts_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_debug_storage_range_at(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_debug_trace_transaction(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_debug_trace_call(const nlohmann::json& request, nlohmann::json& reply);

private:
    asio::io_context& io_context_;
    std::unique_ptr<ethdb::Database>& database_;

    friend class silkrpc::http::RequestHandler;
};

} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_DEBUG_API_HPP_
