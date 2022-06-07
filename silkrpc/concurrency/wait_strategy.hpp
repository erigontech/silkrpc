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

#ifndef SILKRPC_CONCURRENCY_WAIT_STRATEGY_HPP_
#define SILKRPC_CONCURRENCY_WAIT_STRATEGY_HPP_

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <absl/strings/string_view.h>

namespace silkrpc {

struct WaitStrategy {
    virtual ~WaitStrategy() = default;

    virtual void wait_once(uint32_t executed_count) = 0;
};

struct SleepingWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t executed_count) override;

  private:
    inline static const uint32_t kRetries{200};

    uint32_t counter_{kRetries};
};

struct YieldingWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t executed_count) override;

  private:
    inline static const uint32_t kSpinTries{100};

    uint32_t counter_{kSpinTries};
};

struct SpinWaitWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t executed_count) override;

  private:
    void spin_wait();

    inline static const uint32_t kYieldThreshold{10};
    inline static const uint32_t kSleep0EveryHowManyTimes{5};
    inline static const uint32_t kSleep1EveryHowManyTimes{20};

    uint32_t counter_{0};
};

struct BusySpinWaitStrategy : public WaitStrategy {
    void wait_once(uint32_t /*executed_count*/) override {
    }
};

enum class WaitMode {
    blocking,
    sleeping,
    yielding,
    spin_wait,
    busy_spin
};

bool AbslParseFlag(absl::string_view text, WaitMode* wait_mode, std::string* error);
std::string AbslUnparseFlag(WaitMode wait_mode);

std::unique_ptr<WaitStrategy> make_wait_strategy(WaitMode wait_mode);

} // namespace silkrpc

#endif // SILKRPC_CONCURRENCY_WAIT_STRATEGY_HPP_
