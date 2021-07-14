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

#include "evm_executor.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <asio/compose.hpp>
#include <asio/post.hpp>
#include <asio/use_awaitable.hpp>
#include <evmc/evmc.hpp>
#include <intx/intx.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/execution/evm.hpp>
#include <silkworm/state/intra_block_state.hpp>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>

namespace silkrpc {

silkworm::Bytes build_abi_selector(const std::string& signature) {
    const auto signature_hash = hash_of(silkworm::byte_view_of_string(signature));
    return {std::begin(signature_hash.bytes), std::begin(signature_hash.bytes) + 4};
}

const auto kRevertSelector{build_abi_selector("Error(string)")};
const auto kAbiStringOffsetSize{32};

std::optional<std::string> decode_error_reason(const silkworm::Bytes& error_data) {
    if (error_data.size() < kRevertSelector.size() || error_data.substr(0, kRevertSelector.size()) != kRevertSelector) {
        return std::nullopt;
    }

    silkworm::ByteView encoded_msg{error_data.data() + kRevertSelector.size(), error_data.size() - kRevertSelector.size()};
    SILKRPC_TRACE << "silkrpc::decode_error_reason size: " << encoded_msg.size() << " error_message: " << silkworm::to_hex(encoded_msg) << "\n";
    if (encoded_msg.size() < kAbiStringOffsetSize) {
        return std::nullopt;
    }

    const auto offset_uint256{intx::be::unsafe::load<intx::uint256>(encoded_msg.data())};
    SILKRPC_TRACE << "silkrpc::decode_error_reason offset_uint256: " << intx::to_string(offset_uint256) << "\n";
    const auto offset = intx::narrow_cast<uint64_t>(offset_uint256);
    if (encoded_msg.size() < kAbiStringOffsetSize + offset) {
        return std::nullopt;
    }

    const uint64_t message_offset{kAbiStringOffsetSize + offset};
    const auto length_uint256{intx::be::unsafe::load<intx::uint256>(encoded_msg.data() + offset)};
    SILKRPC_TRACE << "silkrpc::decode_error_reason length_uint256: " << intx::to_string(length_uint256) << "\n";
    const auto length = intx::narrow_cast<uint64_t>(length_uint256);
    if (encoded_msg.size() < message_offset + length) {
        return std::nullopt;
    }

    return std::string{std::begin(encoded_msg) + message_offset, std::begin(encoded_msg) + message_offset + length};
}

std::string EVMExecutor::get_error_message(int64_t error_code, const silkworm::Bytes& error_data) {
    SILKRPC_DEBUG << "EVMExecutor::get_error_message error_data: " << silkworm::to_hex(error_data) << "\n";

    std::string error_message;
    switch (error_code) {
        case evmc_status_code::EVMC_FAILURE:
            error_message = "execution failed";
            break;
        case evmc_status_code::EVMC_REVERT:
            error_message = "execution reverted";
            break;
        case evmc_status_code::EVMC_OUT_OF_GAS:
            error_message = "out of gas";
            break;
        case evmc_status_code::EVMC_INVALID_INSTRUCTION:
            error_message = "invalid instruction";
            break;
        case evmc_status_code::EVMC_UNDEFINED_INSTRUCTION:
            error_message = "undefined instruction";
            break;
        case evmc_status_code::EVMC_STACK_OVERFLOW:
            error_message = "stack overflow";
            break;
        case evmc_status_code::EVMC_STACK_UNDERFLOW:
            error_message = "stack underflow";
            break;
        case evmc_status_code::EVMC_BAD_JUMP_DESTINATION:
            error_message = "bad jump destination";
            break;
        case evmc_status_code::EVMC_INVALID_MEMORY_ACCESS:
            error_message = "invalid memory access";
            break;
        case evmc_status_code::EVMC_CALL_DEPTH_EXCEEDED:
            error_message = "call depth exceeded";
            break;
        case evmc_status_code::EVMC_STATIC_MODE_VIOLATION:
            error_message = "static mode violation";
            break;
        case evmc_status_code::EVMC_PRECOMPILE_FAILURE:
            error_message = "precompile failure";
            break;
        case evmc_status_code::EVMC_CONTRACT_VALIDATION_FAILURE:
            error_message = "contract validation failure";
            break;
        case evmc_status_code::EVMC_ARGUMENT_OUT_OF_RANGE:
            error_message = "argument out of range";
            break;
        case evmc_status_code::EVMC_WASM_UNREACHABLE_INSTRUCTION:
            error_message = "wasm unreachable instruction";
            break;
        case evmc_status_code::EVMC_WASM_TRAP:
            error_message = "wasm trap";
            break;
        case evmc_status_code::EVMC_INSUFFICIENT_BALANCE:
            error_message = "insufficient balance";
            break;
        case evmc_status_code::EVMC_INTERNAL_ERROR:
            error_message = "internal error";
            break;
        case evmc_status_code::EVMC_REJECTED:
            error_message = "execution rejected";
            break;
        case evmc_status_code::EVMC_OUT_OF_MEMORY:
            error_message = "out of memory";
            break;
        default:
            error_message = "unknown error code";
    }

    const auto error_reason{decode_error_reason(error_data)};
    if (error_reason) {
        error_message += ": " + *error_reason;
    }

    SILKRPC_DEBUG << "EVMExecutor::get_error_message error_message: " << error_message << "\n";

    return error_message;
}

asio::awaitable<ExecutionResult> EVMExecutor::call(const silkworm::Block& block, const silkworm::Transaction& txn, uint64_t gas) {
    SILKRPC_DEBUG << "Executor::call block: " << block.header.number << " txn: " << &txn << " gas: " << gas << " start\n";

    const auto exec_result = co_await asio::async_compose<decltype(asio::use_awaitable), void(ExecutionResult)>(
        [this, &block, &txn, gas](auto&& self) {
            SILKRPC_TRACE << "Executor::call post block: " << block.header.number << " txn: " << &txn << " gas: " << gas << "\n";
            asio::post(workers_, [this, &block, &txn, gas, self = std::move(self)]() mutable {
                silkworm::IntraBlockState state{buffer_};
                silkworm::EVM evm{block, state, config_};

                SILKRPC_TRACE << "Executor::call execute on EVM block: " << block.header.number << " txn: " << &txn << " start\n";
                const auto result{evm.execute(txn, gas)};
                SILKRPC_TRACE << "Executor::call execute on EVM block: " << block.header.number << " txn: " << &txn << " end\n";

                ExecutionResult exec_result{result.status, result.gas_left, result.data};
                asio::post(*context_.io_context, [exec_result, self = std::move(self)]() mutable {
                    self.complete(exec_result);
                });
            });
        },
        asio::use_awaitable);

    SILKRPC_DEBUG << "Executor::call exec_result: " << exec_result.error_code << " end\n";

    co_return exec_result;
}

} // namespace silkrpc
