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
#include <silkworm/chain/intrinsic_gas.hpp>
#include <silkworm/common/util.hpp>

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
    const auto offset = static_cast<uint64_t>(offset_uint256);
    if (encoded_msg.size() < kAbiStringOffsetSize + offset) {
        return std::nullopt;
    }

    const uint64_t message_offset{kAbiStringOffsetSize + offset};
    const auto length_uint256{intx::be::unsafe::load<intx::uint256>(encoded_msg.data() + offset)};
    SILKRPC_TRACE << "silkrpc::decode_error_reason length_uint256: " << intx::to_string(length_uint256) << "\n";
    const auto length = static_cast<uint64_t>(length_uint256);
    if (encoded_msg.size() < message_offset + length) {
        return std::nullopt;
    }

    return std::string{std::begin(encoded_msg) + message_offset, std::begin(encoded_msg) + message_offset + length};
}

template<typename WorldState, typename VM>

std::string EVMExecutor<WorldState, VM>::get_error_message(int64_t error_code, const silkworm::Bytes& error_data) {
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
            error_message = "invalid opcode";
            break;
        case evmc_status_code::EVMC_STACK_OVERFLOW:
            error_message = "stack overflow";
            break;
        case evmc_status_code::EVMC_STACK_UNDERFLOW:
            error_message = "stack underflow";
            break;
        case evmc_status_code::EVMC_BAD_JUMP_DESTINATION:
            error_message = "invalid jump destination";
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

template<typename WorldState, typename VM>
std::optional<std::string> EVMExecutor<WorldState, VM>::pre_check(const VM& evm, const silkworm::Transaction& txn,
                                                  const intx::uint256 base_fee_per_gas, const intx::uint256 want, const intx::uint128 g0) {
    const WorldState& state{evm.state()};
    const evmc_revision rev{evm.revision()};

    if (rev >= EVMC_LONDON) {
        if (txn.max_fee_per_gas  > 0 || txn.max_priority_fee_per_gas > 0) {
           if (txn.max_fee_per_gas < base_fee_per_gas) {
              std::string from = silkworm::to_hex(*txn.from);
              std::string error = "fee cap less than block base fee: address 0x" + from + ", gasFeeCap: " + intx::to_string(txn.max_fee_per_gas) + " baseFee: " +
                                   intx::to_string(base_fee_per_gas);
              return error;
          }

          if (txn.max_fee_per_gas < txn.max_priority_fee_per_gas) {
              std::string from = silkworm::to_hex(*txn.from);
              std::string error = "tip higher than fee cap: address 0x" + from + ", tip: " + intx::to_string(txn.max_priority_fee_per_gas) + " gasFeeCap: " +
                                   intx::to_string(txn.max_fee_per_gas);
              return error;
          }
       }
    }

    const auto have = state.get_balance(*txn.from);
    if (have < want + txn.value) {
        std::string from = silkworm::to_hex(*txn.from);
        std::string error = "insufficient funds for gas * price + value: address 0x" + from + " have " + intx::to_string(have) + " want " + intx::to_string(want+txn.value);
        return error;
    }

    if (txn.gas_limit < g0) {
        std::string from = silkworm::to_hex(*txn.from);
        std::string error = "intrinsic gas too low: have " + std::to_string(txn.gas_limit) + " want " + intx::to_string(g0);
        return error;
    }

    return std::nullopt;
}

template<typename WorldState, typename VM>
asio::awaitable<ExecutionResult> EVMExecutor<WorldState, VM>::call(const silkworm::Block& block, const silkworm::Transaction& txn) {
    SILKRPC_DEBUG << "Executor::call block: " << block.header.number << " txn: " << &txn << " gas_limit: " << txn.gas_limit << " start\n";

    const auto exec_result = co_await asio::async_compose<decltype(asio::use_awaitable), void(ExecutionResult)>(
        [this, &block, &txn](auto&& self) {
            SILKRPC_TRACE << "EVMExecutor::call post block: " << block.header.number << " txn: " << &txn << "\n";
            asio::post(workers_, [this, &block, &txn, self = std::move(self)]() mutable {
                WorldState state{buffer_};
                VM evm{block, state, config_};

                assert(txn.from.has_value());
                state.access_account(*txn.from);

                const intx::uint256 base_fee_per_gas{evm.block().header.base_fee_per_gas.value_or(0)};
                intx::uint256 want;
                if (txn.max_fee_per_gas  > 0 || txn. max_priority_fee_per_gas > 0) {
                   const intx::uint256 effective_gas_price{txn.effective_gas_price(base_fee_per_gas)};
                   want = txn.gas_limit * effective_gas_price;
                } else {
                   want = 0;
                }
                const evmc_revision rev{evm.revision()};
                const intx::uint128 g0{silkworm::intrinsic_gas(txn, rev >= EVMC_HOMESTEAD, rev >= EVMC_ISTANBUL)};
                assert(g0 <= UINT64_MAX); // true due to the precondition (transaction must be valid)

                const auto error = pre_check(evm, txn, base_fee_per_gas, want, g0);
                if (error) {
                    silkworm::Bytes data{};
                    ExecutionResult exec_result{1000, txn.gas_limit, data, *error};
                    asio::post(*context_.io_context, [exec_result, self = std::move(self)]() mutable {
                        self.complete(exec_result);
                    });
                    return;
                }

                state.subtract_from_balance(*txn.from, want);

                if (txn.to.has_value()) {
                    state.access_account(*txn.to);
                    // EVM itself increments the nonce for contract creation
                    state.set_nonce(*txn.from, txn.nonce + 1);
                }
                for (const silkworm::AccessListEntry& ae : txn.access_list) {
                    state.access_account(ae.account);
                    for (const evmc::bytes32& key : ae.storage_keys) {
                        state.access_storage(ae.account, key);
                    }
                }

                SILKRPC_DEBUG << "EVMExecutor::call execute on EVM txn: " << &txn << " g0: " << static_cast<uint64_t>(g0) << " start\n";
                const auto result{evm.execute(txn, txn.gas_limit - static_cast<uint64_t>(g0))};
                SILKRPC_DEBUG << "EVMExecutor::call execute on EVM txn: " << &txn << " gas_left: " << result.gas_left << " end\n";

                ExecutionResult exec_result{result.status, result.gas_left, result.data};
                asio::post(*context_.io_context, [exec_result, self = std::move(self)]() mutable {
                    self.complete(exec_result);
                });
            });
        },
        asio::use_awaitable);

    SILKRPC_DEBUG << "EVMExecutor::call exec_result: " << exec_result.error_code << " #data: " << exec_result.data.size() << " end\n";

    co_return exec_result;
}

template class EVMExecutor<silkworm::IntraBlockState, silkworm::EVM>;

} // namespace silkrpc
