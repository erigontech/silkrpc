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

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/thread_pool.hpp>
#include <nlohmann/json.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <silkworm/core/silkworm/execution/evm.hpp>
#pragma GCC diagnostic pop
#include <silkworm/state/intra_block_state.hpp>

#include <silkrpc/concurrency/context_pool.hpp>
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

std::string get_op_name(const char* const* names, std::uint8_t opcode);
std::string to_string(intx::uint256 value);
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
    std::optional<std::uint64_t> precompiled_call_gas;
    std::optional<std::uint64_t> call_gas_cap;
    TraceEx trace_ex;
    std::string idx;
    std::uint32_t depth{0};
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
    explicit VmTraceTracer(VmTrace& vm_trace, std::int32_t index = -1) : vm_trace_(vm_trace), transaction_index_{index} {}

    VmTraceTracer(const VmTraceTracer&) = delete;
    VmTraceTracer& operator=(const VmTraceTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_precompiled_run(const evmc::result& result, int64_t gas, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept override {};

private:
    VmTrace& vm_trace_;
    std::int32_t transaction_index_;
    std::stack<std::string> index_prefix_;
    std::stack<std::reference_wrapper<VmTrace>> traces_stack_;
    const char* const* opcode_names_ = nullptr;
    std::stack<std::uint64_t> start_gas_;
    std::stack<TraceMemory> trace_memory_stack_;
};

struct TraceAction {
    std::optional<std::string> call_type;
    evmc::address from;
    std::optional<evmc::address> to;
    std::uint64_t gas{0};
    std::optional<silkworm::Bytes> input;
    std::optional<silkworm::Bytes> init;
    intx::uint256 value{0};
};

struct TraceResult {
    std::optional<evmc::address> address;
    std::optional<silkworm::Bytes> code;
    std::optional<silkworm::Bytes> output;
    std::uint64_t gas_used{0};
};

struct Trace {
    TraceAction trace_action;
    std::optional<TraceResult> trace_result;
    std::int32_t sub_traces{0};
    std::vector<std::uint32_t> trace_address;
    std::optional<std::string> error;
    std::string type;
};

void to_json(nlohmann::json& json, const TraceAction& trace_action);
void to_json(nlohmann::json& json, const TraceResult& trace_result);
void to_json(nlohmann::json& json, const Trace& trace);

template<typename T, typename Container = std::deque<T>>
class iterable_stack: public std::stack<T, Container> {
    using std::stack<T, Container>::c;

public:
    using const_iterator = typename Container::const_iterator;

    const_iterator begin() const { return c.begin(); }
    const_iterator end() const { return std::end(c); }
};

class TraceTracer : public silkworm::EvmTracer {
public:
    explicit TraceTracer(std::vector<Trace>& traces, silkworm::IntraBlockState& initial_ibs)
        : traces_(traces), initial_ibs_(initial_ibs) {}

    TraceTracer(const TraceTracer&) = delete;
    TraceTracer& operator=(const TraceTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_precompiled_run(const evmc::result& result, int64_t gas, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    std::vector<Trace>& traces_;
    silkworm::IntraBlockState& initial_ibs_;
    const char* const* opcode_names_ = nullptr;
    std::uint64_t initial_gas_;
    std::int32_t current_depth_{-1};
    std::set<evmc::address> created_address_;
    iterable_stack<std::uint32_t> index_stack_;
    std::stack<std::uint64_t> start_gas_;
};

struct DiffValue {
    std::optional<std::string> from;
    std::optional<std::string> to;
};

using DiffStorage = std::map<std::string, DiffValue>;

struct StateDiffEntry {
    DiffValue balance;
    DiffValue code;
    DiffValue nonce;
    DiffStorage storage;
};

using StateDiff = std::map<std::string, StateDiffEntry>;

struct StateAddress {
    intx::uint256 balance{0};
    uint64_t nonce{0};
    silkworm::Bytes code;
};

class StateAddresses {
public:
    explicit StateAddresses(silkworm::IntraBlockState& initial_ibs) :
        initial_ibs_(initial_ibs) {}

    StateAddresses(const StateAddresses&) = delete;
    StateAddresses& operator=(const StateAddresses&) = delete;

    bool exists(const evmc::address& address) const noexcept {return initial_ibs_.exists(address);}

    intx::uint256 get_balance(const evmc::address& address) const noexcept;
    void set_balance(const evmc::address& address, const intx::uint256& value) noexcept {balances_[address] = value;}
    bool balance_exists(const evmc::address& address) const noexcept {return balances_.find(address) != balances_.end();}

    uint64_t get_nonce(const evmc::address& address) const noexcept;
    void set_nonce(const evmc::address& address, uint64_t nonce) noexcept {nonces_[address] = nonce;}
    bool nonce_exists(const evmc::address& address) const noexcept {return nonces_.find(address) != nonces_.end();}

    silkworm::ByteView get_code(const evmc::address& address) const noexcept;
    void set_code(const evmc::address& address, silkworm::ByteView code) noexcept {codes_[address] = silkworm::Bytes{code};}
    bool code_exists(const evmc::address& address) const noexcept {return codes_.find(address) != codes_.end();}

private:
    std::map<evmc::address, intx::uint256> balances_;
    std::map<evmc::address, uint64_t> nonces_;
    std::map<evmc::address, silkworm::Bytes> codes_;
    silkworm::IntraBlockState& initial_ibs_;
};

void to_json(nlohmann::json& json, const DiffValue& dn);
void to_json(nlohmann::json& json, const StateDiffEntry& state_diff);

class StateDiffTracer : public silkworm::EvmTracer {
public:
    explicit StateDiffTracer(StateDiff &state_diff, StateAddresses& state_addresses) :
        state_diff_(state_diff), state_addresses_(state_addresses) {}

    StateDiffTracer(const StateDiffTracer&) = delete;
    StateDiffTracer& operator=(const StateDiffTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_precompiled_run(const evmc::result& result, int64_t gas, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    StateDiff& state_diff_;
    StateAddresses& state_addresses_;
    // silkworm::IntraBlockState& initial_ibs_;
    std::map<evmc::address, std::set<std::string>> diff_storage_;
    std::map<evmc::address, silkworm::ByteView> code_;
    const char* const* opcode_names_ = nullptr;
};

struct TraceCallTraces {
    std::string output{"0x"};
    std::optional<evmc::bytes32> transaction_hash;
    std::optional<StateDiff> state_diff;
    std::vector<Trace> trace;
    std::optional<VmTrace> vm_trace;
};

struct TraceCallResult {
    TraceCallTraces traces;
    std::optional<std::string> pre_check_error{std::nullopt};
};

void to_json(nlohmann::json& json, const TraceCallTraces& result);
void to_json(nlohmann::json& json, const TraceCallResult& result);

class IntraBlockStateTracer : public silkworm::EvmTracer {
public:
    explicit IntraBlockStateTracer(StateAddresses& state_addresses) :
        state_addresses_{state_addresses} {}

    IntraBlockStateTracer(const IntraBlockStateTracer&) = delete;
    IntraBlockStateTracer& operator=(const IntraBlockStateTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override {};
    void on_instruction_start(uint32_t pc , const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_precompiled_run(const evmc::result& result, int64_t gas, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    StateAddresses& state_addresses_;
};

template<typename WorldState = silkworm::IntraBlockState, typename VM = silkworm::EVM>
class TraceCallExecutor {
public:
    explicit TraceCallExecutor(asio::io_context& io_context, const core::rawdb::DatabaseReader& database_reader, asio::thread_pool& workers, const TraceConfig& config = DEFAULT_TRACE_CONFIG)
    : io_context_(io_context), database_reader_(database_reader), workers_{workers}, config_{config} {}
    virtual ~TraceCallExecutor() {}

    TraceCallExecutor(const TraceCallExecutor&) = delete;
    TraceCallExecutor& operator=(const TraceCallExecutor&) = delete;

    asio::awaitable<std::vector<TraceCallResult>> execute(const silkworm::Block& block);
    asio::awaitable<TraceCallResult> execute(const silkworm::Block& block, const silkrpc::Call& call);
    asio::awaitable<TraceCallResult> execute(const silkworm::Block& block, const silkrpc::Transaction& transaction) {
        return execute(block.header.number-1, block, transaction, transaction.transaction_index);
    }

private:
    asio::awaitable<TraceCallResult> execute(std::uint64_t block_number, const silkworm::Block& block, const silkrpc::Transaction& transaction, std::int32_t = -1);

    asio::io_context& io_context_;
    const core::rawdb::DatabaseReader& database_reader_;
    asio::thread_pool& workers_;
    const TraceConfig& config_;
};
} // namespace silkrpc::trace

#endif  // SILKRPC_CORE_EVM_TRACE_HPP_
