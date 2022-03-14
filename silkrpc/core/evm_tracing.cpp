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

#include "evm_tracing.hpp"

#include <memory>
#include <stack>
#include <string>

#include <evmc/hex.hpp>
#include <evmc/instructions.h>
#include <intx/intx.hpp>
#include <silkworm/third_party/evmone/lib/evmone/execution_state.hpp>
#include <silkworm/third_party/evmone/lib/evmone/instructions.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/evm_executor.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/json/types.hpp>

namespace silkrpc::trace {

void from_json(const nlohmann::json& json, TraceConfig& tc) {
    json.at("disableStorage").get_to(tc.disableStorage);
    json.at("disableMemory").get_to(tc.disableMemory);
    json.at("disableStack").get_to(tc.disableStack);
}

std::ostream& operator<<(std::ostream& out, const TraceConfig& tc) {
    out << "disableStorage: " << std::boolalpha << tc.disableStorage;
    out << " disableMemory: " << std::boolalpha << tc.disableMemory;
    out << " disableStack: " << std::boolalpha << tc.disableStack;

    return out;
}

void to_json(nlohmann::json& json, const Trace& trace) {
    json["failed"] = trace.failed;
    json["gas"] = trace.gas;
    json["returnValue"] = trace.return_value;

    const auto& config = trace.trace_config;
    json["structLogs"] = nlohmann::json::array();
    for (auto& log : trace.trace_logs) {
        nlohmann::json entry;

        entry["depth"] = log.depth;
        entry["gas"] = log.gas;
        entry["gasCost"] = log.gas_cost;
        entry["op"] = log.op;
        entry["pc"] = log.pc;
        if (!config.disableStack) {
            entry["stack"] = log.stack;
        }
        if (!config.disableMemory) {
            entry["memory"] = log.memory;
        }
        if (!config.disableStorage && !log.storage.empty()) {
            entry["storage"] = log.storage;
        }
        if (log.error) {
            entry["error"] = nlohmann::json::object();
        }
        json["structLogs"].push_back(entry);
    }
}

std::string get_opcode_name(const char* const* names, std::uint8_t opcode) {
    const auto name = names[opcode];
    return (name != nullptr) ?name : "opcode 0x" + evmc::hex(opcode) + " not defined";
}

static std::string EMPTY_MEMORY(64, '0');

void output_stack(std::vector<std::string>& vect, const evmone::Stack& stack) {
    vect.reserve(stack.size());
    const auto top = stack.size() - 1;
    for (int i = top; i >= 0; --i) {
        vect.push_back("0x" + intx::to_string(stack[i], 16));
    }
}

void output_memory(std::vector<std::string>& vect, const evmone::Memory& memory) {
    std::size_t len = 32;
    vect.reserve(memory.size() / len);

    const auto data = memory.data();
    for (std::size_t start = 0; start < memory.size(); start += len) {
        std::string entry{evmc::hex({data + start, len})};
        vect.push_back(entry);
    }
}

void insert_error(TraceLog& log, evmc_status_code status_code) {
    switch(status_code) {
    case evmc_status_code::EVMC_FAILURE:
    case evmc_status_code::EVMC_UNDEFINED_INSTRUCTION:
    case evmc_status_code::EVMC_OUT_OF_GAS:
        log.error = true;
        break;
    default:
        log.error = false;
        break;
    }
}

void DebugTracer::on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept {
    if (opcode_names_ == nullptr) {
        opcode_names_ = evmc_get_instruction_names_table(rev);
    }

    start_gas_ = msg.gas;
    evmc::address recipient(msg.recipient);
    evmc::address sender(msg.sender);
    SILKRPC_DEBUG << "on_execution_start: gas: " << std::dec << msg.gas
        << " depth: " << msg.depth
        << " recipient: " << recipient
        << " sender: " << sender
        << "\n";
}

void DebugTracer::on_instruction_start(uint32_t pc, const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept {
    assert(execution_state.msg);
    evmc::address recipient(execution_state.msg->recipient);
    evmc::address sender(execution_state.msg->sender);

    const auto opcode = execution_state.code[pc];
    auto opcode_name = get_opcode_name(opcode_names_, opcode);

    SILKRPC_DEBUG << "on_instruction_start:"
        << " pc: " << std::dec << pc
        << " opcode: 0x" << std::hex << evmc::hex(opcode)
        << " opcode_name: " << opcode_name
        << " recipient: " << recipient
        << " sender: " << sender
        << " execution_state: {"
        << "   gas_left: " << std::dec << execution_state.gas_left
        << "   status: " << execution_state.status
        << "   msg.gas: " << std::dec << execution_state.msg->gas
        << "   msg.depth: " << std::dec << execution_state.msg->depth
        << "}\n";

    bool output_storage = false;
    if (!config_.disableStorage) {
        if (opcode_name == "SLOAD" && execution_state.stack.size() > 0) {
            const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[0]));
            const auto value = intra_block_state.get_current_storage(recipient, address);

            storage_[recipient][silkworm::to_hex(address)] = silkworm::to_hex(value);
            output_storage = true;
        } else if (opcode_name == "SSTORE" && execution_state.stack.size() > 1) {
            const auto address = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[0]));
            const auto value = silkworm::bytes32_from_hex(intx::hex(execution_state.stack[1]));
            storage_[recipient][silkworm::to_hex(address)] = silkworm::to_hex(value);
            output_storage = true;
        }
    }

    std::vector<std::string> current_memory;
    if (!config_.disableMemory) {
        output_memory(current_memory, execution_state.memory);
    }

    if (logs_.size() > 0) {
        auto& log = logs_[logs_.size() - 1];
        auto depth = log.depth;
        if (depth == execution_state.msg->depth + 1) {
            auto gas_cost = log.gas - execution_state.gas_left;
            log.gas_cost = gas_cost;

            if (!config_.disableMemory) {
                auto& memory = log.memory;
                for (int idx = memory.size(); idx < current_memory.size(); idx++) {
                    memory.push_back(EMPTY_MEMORY);
                }
            }
        } else if (depth == execution_state.msg->depth) {
            log.gas_cost = start_gas_;
        }
    }

    TraceLog log;
    log.pc = pc;
    log.op = opcode_name == "KECCAK256" ? "SHA3" : opcode_name; // TODO(sixtysixter) for RPCDAEMON compatibility
    log.gas = execution_state.gas_left;
    log.depth = execution_state.msg->depth + 1;
    if (!config_.disableStack) {
        output_stack(log.stack, execution_state.stack);
    }
    if (!config_.disableMemory) {
        log.memory = current_memory;
    }
    if (output_storage) {
        for (const auto &entry : storage_[recipient]) {
            log.storage[entry.first] = entry.second;
        }
    }
    insert_error(log, execution_state.status);

    logs_.push_back(log);
}

void DebugTracer::on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept {
    if (logs_.size() > 0) {
        auto& log = logs_[logs_.size() - 1];

        insert_error(log, result.status_code);

        switch (result.status_code) {
        case evmc_status_code::EVMC_REVERT:
        case evmc_status_code::EVMC_OUT_OF_GAS:
            log.gas_cost = 0;
            break;

        case evmc_status_code::EVMC_UNDEFINED_INSTRUCTION:
            log.gas_cost = start_gas_ - log.gas;
            break;

        default:
            log.gas_cost = log.gas - result.gas_left;
            break;
        }
    }

    SILKRPC_DEBUG << "on_execution_end:"
        << " result.status_code: " << result.status_code
        << " start_gas: " << std::dec << start_gas_
        << " gas_left: " << std::dec << result.gas_left
        << "\n";
}

template<typename WorldState, typename VM>
asio::awaitable<TraceExecutorResult> TraceExecutor<WorldState, VM>::execute(const silkworm::Block& block, const silkrpc::Call& call) {
    silkrpc::Transaction transaction{call.to_transaction()};
    auto result = co_await execute(block.header.number, block, transaction, -1);
    co_return result;
}

template<typename WorldState, typename VM>
asio::awaitable<TraceExecutorResult> TraceExecutor<WorldState, VM>::execute(std::uint64_t block_number, const silkworm::Block& block,
        const silkrpc::Transaction& transaction, std::int32_t index) {
    SILKRPC_DEBUG << "execute: "
        << " block_number: " << block_number
        << " transaction: {" << transaction << "}"
        << " index: " << std::dec << index
        << " config: " << config_
        << "\n";

    const auto chain_id = co_await core::rawdb::read_chain_id(database_reader_);

    const auto chain_config_ptr = silkworm::lookup_chain_config(chain_id);

    EVMExecutor<WorldState, VM> executor{context_, database_reader_, *chain_config_ptr, workers_, block_number};

    for (auto idx = 0; idx < index; idx++) {
        silkrpc::Transaction txn{block.transactions[idx]};
        txn.recover_sender();
        const auto execution_result = co_await executor.call(block, txn);
    }

    TraceExecutorResult result;
    auto& trace = result.trace;

    trace.trace_config = config_;

    auto tracer = std::make_shared<trace::DebugTracer>(trace.trace_logs, config_);
    const auto execution_result = co_await executor.call(block, transaction, tracer);

    if (execution_result.pre_check_error) {
        result.pre_check_error = "tracing failed: " + execution_result.pre_check_error.value();
    } else {
        trace.failed = execution_result.error_code != evmc_status_code::EVMC_SUCCESS;
        trace.gas = transaction.gas_limit - execution_result.gas_left;
        trace.return_value = silkworm::to_hex(execution_result.data);
    }

    co_return result;
}

template class TraceExecutor<>;

} // namespace silkrpc::trace
