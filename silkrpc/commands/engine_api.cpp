/*
    Copyright 2022 The Silkrpc Authors

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

#include "engine_api.hpp"

#include <string>

namespace silkrpc::commands {

asio::awaitable<void> EngineRpcApi::handle_engine_get_payload_v1(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request.at("params");

    if (params.size() != 1) {
        auto error_msg = "invalid engine_getPayloadV1 params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request.at("id"), 100, error_msg);
        co_return;
    }
    // Coverage data result malformed in cmd/unit_test if we use a try/catch here
    #ifndef BUILD_COVERAGE
    try {
    #endif
        const auto payload_id = params[0].get<std::string>();
        const auto payload_number = std::stoul(payload_id, 0, 16);
        reply = co_await backend_->engine_get_payload_v1(payload_number);
    #ifndef BUILD_COVERAGE
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request.at("id"), 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request.at("id"), 100, "unexpected exception");
    }
    #endif
}


} // namespace silkrpc::commands
