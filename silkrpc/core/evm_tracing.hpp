/*
   Copyright 2021 The Silkrpc Authors

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

#ifndef SILKRPC_CORE_EVM_TRACING_HPP_
#define SILKRPC_CORE_EVM_TRACING_HPP_

#include <map>
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
    bool disableStorage{false};
    bool disableMemory{false};
    bool disableStack{false};
};

static const TraceConfig DEFAULT_TRACE_CONFIG{false, false, false};

void from_json(const nlohmann::json& json, TraceConfig& tc);
std::ostream& operator<<(std::ostream& out, const TraceConfig& tc);

using Storage = std::map<std::string, std::string>;

struct TraceLog {
    std::uint32_t pc;
    std::string op;
    std::int64_t gas;
    std::int64_t gas_cost;
    std::uint32_t depth;
    bool error{false};
    std::vector<std::string> memory;
    std::vector<std::string> stack;
    Storage storage;
};

class DebugTracer : public silkworm::EvmTracer {
public:
    explicit DebugTracer(std::vector<TraceLog>& logs, const TraceConfig& config = {})
        : logs_(logs), config_(config) {}

    DebugTracer(const DebugTracer&) = delete;
    DebugTracer& operator=(const DebugTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc, const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override;

private:
    std::vector<TraceLog>& logs_;
    const TraceConfig& config_;
    std::map<evmc::address, Storage> storage_;
    const char* const* opcode_names_ = nullptr;
    std::int64_t start_gas_{0};
};

class NullTracer : public silkworm::EvmTracer {
public:
    NullTracer() {}

    NullTracer(const NullTracer&) = delete;
    NullTracer& operator=(const NullTracer&) = delete;

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override {};
    void on_instruction_start(uint32_t pc, const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override {};

    std::int64_t get_end_gas() const {return 0;}
};

struct Trace {
    bool failed;
    std::int64_t gas;
    std::string return_value;
    std::vector<TraceLog> trace_logs;

    TraceConfig trace_config;
};

void to_json(nlohmann::json& json, const Trace& trace);
void to_json(nlohmann::json& json, const std::vector<Trace>& traces);

struct TraceExecutorResult {
    Trace trace;
    std::optional<std::string> pre_check_error{std::nullopt};
};

template<typename WorldState = silkworm::IntraBlockState, typename VM = silkworm::EVM>
class TraceExecutor {
public:
    explicit TraceExecutor(const Context& context, const core::rawdb::DatabaseReader& database_reader, asio::thread_pool& workers, const TraceConfig& config = DEFAULT_TRACE_CONFIG)
    : context_(context), database_reader_(database_reader), workers_{workers}, config_{config} {}
    virtual ~TraceExecutor() {}

    TraceExecutor(const TraceExecutor&) = delete;
    TraceExecutor& operator=(const TraceExecutor&) = delete;

    asio::awaitable<std::vector<Trace>> execute(const silkworm::Block& block_with_hash);
    asio::awaitable<TraceExecutorResult> execute(const silkworm::Block& block, const silkrpc::Call& call);
    asio::awaitable<TraceExecutorResult> execute(const silkworm::Block& block, const silkrpc::Transaction& transaction) {
        return execute(block.header.number-1, block, transaction, transaction.transaction_index);
    }

private:
    asio::awaitable<TraceExecutorResult> execute(std::uint64_t block_number, const silkworm::Block& block, const silkrpc::Transaction& transaction, std::int32_t = -1);

    const Context& context_;
    const core::rawdb::DatabaseReader& database_reader_;
    asio::thread_pool& workers_;
    const TraceConfig& config_;
};
} // namespace silkrpc::trace

#endif  // SILKRPC_CORE_EVM_TRACING_HPP_
