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
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/types/execution_payload.hpp>
#include <silkrpc/ethdb/transaction_database.hpp>

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
    #ifndef BUILD_COVERAGE
    try {
    #endif
        const auto payload_id = params[0].get<std::string>();
        reply = co_await backend_->engine_get_payload_v1(std::stoul(payload_id, 0, 16));
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

asio::awaitable<void> EngineRpcApi::handle_engine_new_payload_v1(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request.at("params");

    if (params.size() != 1) {
        auto error_msg = "invalid engine_newPayloadV1 params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request.at("id"), 100, error_msg);
        co_return;
    }
    #ifndef BUILD_COVERAGE
    try {
    #endif
        const auto payload = params[0].get<ExecutionPayload>();
        reply = co_await backend_->engine_new_payload_v1(payload);
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


asio::awaitable<void> EngineRpcApi::handle_engine_transition_configuration_v1(const nlohmann::json& request, nlohmann::json& reply){
    auto params = request.at("params");
    // params = [TransitionConfiguration]
    if(params.size() != 1){
        auto error_msg = "invalid engine_transitionConfigurationV1 params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request.at("id"), 100, error_msg);
        co_return;
    }
    #ifndef BUILD_COVERAGE
    try{
        #endif
        const auto cl_configuration = params[0].get<TransitionConfiguration>();

        auto tx = co_await database_->begin();
        ethdb::TransactionDatabase tx_database{*tx};
        
        const auto chain_config{co_await silkrpc::core::rawdb::read_chain_config(tx_database)};
        SILKRPC_DEBUG << "chain config: " << chain_config << "\n";

        auto config = silkworm::ChainConfig::from_json(chain_config.config);

        if(cl_configuration.terminal_block_number != 0){
            SILKRPC_ERROR << "consensus layer has the wrong terminal block number expected zero but instead got: " << cl_configuration.terminal_block_number << "\n";
            reply = make_json_error(request.at("id"), 100, "consensus layer terminal block number is not zero");
        }

        if(config->terminal_total_difficulty != std::nullopt){
            SILKRPC_ERROR << "execution layer does not have terminal total difficulty";
            reply = make_json_error(request.at("id"), 100, "execution layer does not have terminal total difficulty");
        }

        if(config->terminal_total_difficulty != cl_configuration.terminal_total_difficulty){
            SILKRPC_ERROR << "execution layer has the incorrect terminal total difficulty, expected: " << cl_configuration.terminal_total_difficulty << " got: " << *config->terminal_total_difficulty << "\n";
            reply = make_json_error(request.at("id"), 100, "incorrect terminal total difficulty");
        }

        if(config->terminal_block_hash != cl_configuration.terminal_block_hash){
            SILKRPC_ERROR << "executioono layer has the incorrect terminal block hash, expected: " << cl_configuration.terminal_block_hash << " got: " << *config->terminal_block_hash << "\n";
            reply = make_json_error(request.at("id"), 100, "incorrect terminal block hash");
        }

        TransitionConfiguration transition_configuration{
            .terminal_total_difficulty = *config->terminal_total_difficulty, 
            .terminal_block_hash = *config->terminal_block_hash, 
            .terminal_block_number = 0
        };
        reply = transition_configuration;

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
