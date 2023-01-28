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

#ifndef SILKRPC_COMMANDS_OTS_API_HPP_
#define SILKRPC_COMMANDS_OTS_API_HPP_

#include <memory>
#include <vector>


#include <silkrpc/config.hpp> // NOLINT(build/include_order)

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <silkrpc/concurrency/context_pool.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/log.hpp>
#include <silkrpc/ethbackend/backend.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/database.hpp>
#include <silkrpc/ethdb/kv/state_cache.hpp>

namespace silkrpc::http { class RequestHandler; }

namespace silkrpc::commands {

class OtsRpcApi {
public:
    explicit OtsRpcApi(Context& context): database_(context.database()), state_cache_(context.state_cache()) {}
    virtual ~OtsRpcApi() = default;

    OtsRpcApi(const OtsRpcApi&) = delete;
    OtsRpcApi& operator=(const OtsRpcApi&) = delete;

protected:
    boost::asio::awaitable<void> handle_ots_get_api_level(const nlohmann::json& request, nlohmann::json& reply);
    boost::asio::awaitable<void> handle_ots_has_code(const nlohmann::json& request, nlohmann::json& reply);

    std::unique_ptr<ethdb::Database>& database_;
    std::shared_ptr<ethdb::kv::StateCache>& state_cache_;
    friend class silkrpc::http::RequestHandler;
};
} // namespace silkrpc::commands

#endif  // SILKRPC_COMMANDS_OTS_API_HPP_