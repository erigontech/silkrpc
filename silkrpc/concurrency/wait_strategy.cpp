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

#include "wait_strategy.hpp"

#include <utility>

#include <absl/strings/str_cat.h>

namespace silkrpc {

void SleepingWaitStrategy::wait_once(uint32_t executed_count) {
    if (executed_count > 0) {
        if (counter_ != kRetries) {
            counter_ = kRetries;
        }
        return;
    }

    if (counter_ > 100) {
        --counter_;
    } else if (counter_ > 0) {
        --counter_;
        std::this_thread::yield();
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
}

void YieldingWaitStrategy::wait_once(uint32_t executed_count) {
    if (executed_count > 0) {
        if (counter_ != kSpinTries) {
            counter_ = kSpinTries;
        }
        return;
    }

    if (counter_ == 0) {
        std::this_thread::yield();
    } else {
        --counter_;
    }
}

void SpinWaitWaitStrategy::wait_once(uint32_t executed_count) {
    if (executed_count > 0) {
        if (counter_ != 0) {
            counter_ = 0;
        }
        return;
    }

    if (counter_ > kYieldThreshold) {
        auto delta = counter_ - kYieldThreshold;
        if (delta % kSleep1EveryHowManyTimes == kSleep1EveryHowManyTimes - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else if (delta % kSleep0EveryHowManyTimes == kSleep0EveryHowManyTimes - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
        } else {
            std::this_thread::yield();
        }
    } else {
        for (auto i{0}; i < (4 << counter_); i++) {
            spin_wait();
        }
    }
}

void SpinWaitWaitStrategy::spin_wait() {
    asm volatile
    (
        "rep\n"
        "nop"
    );
}

bool AbslParseFlag(absl::string_view text, WaitMode* wait_mode, std::string* error) {
    if (text == "blocking") {
        *wait_mode = WaitMode::blocking;
        return true;
    }
    if (text == "sleeping") {
        *wait_mode = WaitMode::sleeping;
        return true;
    }
    if (text == "yielding") {
        *wait_mode = WaitMode::yielding;
        return true;
    }
    if (text == "spin_wait") {
        *wait_mode = WaitMode::spin_wait;
        return true;
    }
    if (text == "busy_spin") {
        *wait_mode = WaitMode::busy_spin;
        return true;
    }
    *error = "unknown value for WaitMode";
    return false;
}

std::string AbslUnparseFlag(WaitMode wait_mode) {
    switch (wait_mode) {
        case WaitMode::blocking: return "blocking";
        case WaitMode::sleeping: return "sleeping";
        case WaitMode::yielding: return "yielding";
        case WaitMode::spin_wait: return "spin_wait";
        case WaitMode::busy_spin: return "busy_spin";
        default: return absl::StrCat(wait_mode);
    }
}

std::unique_ptr<WaitStrategy> make_wait_strategy(WaitMode wait_mode) {
    switch (wait_mode) {
        case WaitMode::yielding:
            return std::make_unique<YieldingWaitStrategy>();
        case WaitMode::sleeping:
            return std::make_unique<SleepingWaitStrategy>();
        case WaitMode::spin_wait:
            return std::make_unique<SpinWaitWaitStrategy>();
        case WaitMode::busy_spin:
            return std::make_unique<BusySpinWaitStrategy>();
        default:
            return nullptr;
    }
}

} // namespace silkrpc
