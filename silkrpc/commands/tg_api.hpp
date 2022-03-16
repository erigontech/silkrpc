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

#ifndef SILKRPC_COMMANDS_TG_API_HPP_
#define SILKRPC_COMMANDS_TG_API_HPP_

#include <memory>

#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/ethdb/database.hpp>

namespace silkrpc::http { class RequestHandler; }

namespace silkrpc::commands {

class TurboGethRpcApi {
public:
    explicit TurboGethRpcApi(Context& context) : database_(context.database), context_(context) {}
    virtual ~TurboGethRpcApi() {}

    TurboGethRpcApi(const TurboGethRpcApi&) = delete;
    TurboGethRpcApi& operator=(const TurboGethRpcApi&) = delete;

protected:
    asio::awaitable<void> handle_tg_get_header_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_tg_get_header_by_number(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_tg_get_logs_by_hash(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_tg_forks(const nlohmann::json& request, nlohmann::json& reply);
    asio::awaitable<void> handle_tg_issuance(const nlohmann::json& request, nlohmann::json& reply);

private:
    std::unique_ptr<ethdb::Database>& database_;
    Context& context_;

    friend class silkrpc::http::RequestHandler;
};

} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_TG_API_HPP_
