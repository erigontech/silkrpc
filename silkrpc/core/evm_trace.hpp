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

#ifndef SILKRPC_CORE_EVM_TRACE_HPP_
#define SILKRPC_CORE_EVM_TRACE_HPP_

#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>
#include <nlohmann/json.hpp>

#include <silkworm/core/silkworm/execution/evm.hpp>
#include <silkworm/state/intra_block_state.hpp>

#include <silkrpc/context_pool.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/call.hpp>
#include <silkrpc/types/transaction.hpp>

namespace silkrpc::trace {

struct TraceConfig {
    bool vm_trace{false};
    bool trace{false};
    bool state_diff{false};
};

static const TraceConfig DEFAULT_TRACE_CONFIG{false, false, false};

std::ostream& operator<<(std::ostream& out, const TraceConfig& tc);

struct TraceStorage {
    std::string key;
    std::string value;
};

struct TraceMemory {
    std::uint64_t offset{0};
    std::uint64_t len{0};
    std::string data;
};

struct TraceEx {
    std::optional<TraceMemory> memory;
    std::vector<std::string> stack;
    std::optional<TraceStorage> storage;
    std::uint64_t used{0};
};

struct VmTrace;

struct TraceOp {
    std::uint64_t gas_cost{0};
    TraceEx trace_ex;
    std::uint32_t idx;
    std::uint8_t op_code;
    std::string op_name;
    std::uint32_t pc;
    std::shared_ptr<VmTrace> sub;
};

struct VmTrace {
    std::string code{"0x"};
    std::vector<TraceOp> ops;
};

void to_json(nlohmann::json& json, const VmTrace& vm_trace);
void to_json(nlohmann::json& json, const TraceOp& trace_op);
void to_json(nlohmann::json& json, const TraceEx& trace_ex);
void to_json(nlohmann::json& json, const TraceMemory& trace_memory);
void to_json(nlohmann::json& json, const TraceStorage& trace_storage);


void copy_stack(std::uint8_t op_code, const evmone::uint256* stack, std::vector<std::string>& trace_stack);
void copy_memory(const evmone::Memory& memory, std::optional<TraceMemory>& trace_memory);
void copy_store(std::uint8_t op_code, const evmone::uint256* stack, std::optional<TraceStorage>& trace_storage);
void copy_memory_offset_len(std::uint8_t op_code, const evmone::uint256* stack, std::optional<TraceMemory>& trace_memory);
void push_memory_offset_len(std::uint8_t op_code, const evmone::uint256* stack, std::stack<TraceMemory>& tms);

class VmTraceTracer : public silkworm::EvmTracer {
public:
    explicit VmTraceTracer(VmTrace& vm_trace) : vm_trace_(vm_trace) {}

    VmTraceTracer(const VmTraceTracer&) = delete;
    VmTraceTracer& operator=(const VmTraceTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    VmTrace& vm_trace_;
    std::stack<std::reference_wrapper<VmTrace>> trace_stack_;
    const char* const* opcode_names_ = nullptr;
    std::int32_t next_index_{0};
    std::stack<std::uint64_t> start_gas_;
    std::stack<TraceMemory> trace_memory_stack_;
};

struct TraceAction {
    std::optional<std::string> call_type;
    evmc::address from;
    std::optional<evmc::address> to;
    std::uint64_t gas{0};
    silkworm::Bytes input{};
    silkworm::Bytes init{};
    silkworm::Bytes value{};
};

struct TraceResult {
    evmc::address address;
    silkworm::Bytes code{};
    std::uint64_t gas_used;
};

struct Trace {
    TraceAction trace_action;
    std::optional<TraceResult> trace_result;
    std::int32_t sub_traces{0};
    std::vector<evmc::address> trace_address;
    std::optional<std::string> error;
    std::string type;
};

void to_json(nlohmann::json& json, const TraceAction& trace_action);
void to_json(nlohmann::json& json, const TraceResult& trace_result);
void to_json(nlohmann::json& json, const Trace& trace);

class TraceTracer : public silkworm::EvmTracer {
public:
    explicit TraceTracer(Trace& trace) : trace_(trace) {}

    TraceTracer(const TraceTracer&) = delete;
    TraceTracer& operator=(const TraceTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    Trace& trace_;
    const char* const* opcode_names_ = nullptr;
    std::uint64_t start_gas_{0};
};

struct DiffBalanceEntry {
    evmc::address from;
    evmc::address to;
};

struct DiffCodeEntry {
    std::string from;
    std::string to;
};

using DiffBalance = std::map<std::string, DiffBalanceEntry>;
using DiffCode = std::map<std::string, DiffCodeEntry>;
using DiffNonce = std::map<std::string, std::string>;
using DiffStorage = std::map<std::string, std::string>;

struct StateDiffEntry {
    DiffBalance balance;
    DiffCode code;
    DiffNonce nonce;
    DiffStorage storage;
};

using StateDiff = std::map<std::string, StateDiffEntry>;

void to_json(nlohmann::json& json, const DiffBalanceEntry& dfe);
void to_json(nlohmann::json& json, const DiffCodeEntry& dce);
void to_json(nlohmann::json& json, const StateDiffEntry& state_diff);

class StateDiffTracer : public silkworm::EvmTracer {
public:
    explicit StateDiffTracer(StateDiff &state_diff) : state_diff_(state_diff) {}

    StateDiffTracer(const StateDiffTracer&) = delete;
    StateDiffTracer& operator=(const StateDiffTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    StateDiff& state_diff_;
    const char* const* opcode_names_ = nullptr;
    std::uint64_t start_gas_{0};
};

struct TraceCallTraces {
    std::string output{"0x"};
    std::optional<StateDiff> state_diff;
    std::optional<Trace> trace;
    std::optional<VmTrace> vm_trace;
};

struct TraceCallResult {
    TraceCallTraces traces;
    std::optional<std::string> pre_check_error{std::nullopt};
};

void to_json(nlohmann::json& json, const TraceCallTraces& result);

template<typename WorldState = silkworm::IntraBlockState, typename VM = silkworm::EVM>
class TraceCallExecutor {
public:
    explicit TraceCallExecutor(const Context& context, const core::rawdb::DatabaseReader& database_reader, asio::thread_pool& workers, const TraceConfig& config = DEFAULT_TRACE_CONFIG)
    : context_(context), database_reader_(database_reader), workers_{workers}, config_{config} {}
    virtual ~TraceCallExecutor() {}

    TraceCallExecutor(const TraceCallExecutor&) = delete;
    TraceCallExecutor& operator=(const TraceCallExecutor&) = delete;

    asio::awaitable<TraceCallResult> execute(const silkworm::Block& block, const silkrpc::Call& call);

private:
    asio::awaitable<TraceCallResult> execute(std::uint64_t block_number, const silkworm::Block& block, const silkrpc::Transaction& transaction, std::int32_t = -1);

    const Context& context_;
    const core::rawdb::DatabaseReader& database_reader_;
    asio::thread_pool& workers_;
    const TraceConfig& config_;
};
} // namespace silkrpc::trace

#endif  // SILKRPC_CORE_EVM_TRACE_HPP_
