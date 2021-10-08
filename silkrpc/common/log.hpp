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

#ifndef SILKRPC_COMMON_LOG_HPP_
#define SILKRPC_COMMON_LOG_HPP_

#include <absl/strings/string_view.h>

#include <string>

#include <silkworm/common/log.hpp>

namespace silkworm {

bool AbslParseFlag(absl::string_view text, LogLevel* level, std::string* error);
std::string AbslUnparseFlag(LogLevel level);

} // namespace silkworm

namespace silkrpc {

using Logger = silkworm::log_;
using LogLevel = silkworm::LogLevel;

#define LOG(level_) if ((level_) < silkworm::log_verbosity_) {} else silkworm::log_(level_) << " " // NOLINT

// LogTrace, LogDebug, LogInfo, LogWarn, LogError, LogCritical, LogNone
#define SILKRPC_TRACE LOG(LogLevel::Trace)
#define SILKRPC_DEBUG LOG(LogLevel::Debug)
#define SILKRPC_INFO  LOG(LogLevel::Info)
#define SILKRPC_WARN  LOG(LogLevel::Warn)
#define SILKRPC_ERROR LOG(LogLevel::Error)
#define SILKRPC_CRIT  LOG(LogLevel::Critical)
#define SILKRPC_LOG   LOG(LogLevel::None)

#define SILKRPC_LOG_VERBOSITY(level_) (silkworm::log_verbosity_ = (level_))

#define SILKRPC_LOG_THREAD(log_thread_) (silkworm::log_thread_enabled_ = (log_thread_))

#define SILKRPC_LOG_STREAMS(stream1_, stream2_) silkworm::log_set_streams_((stream1_), (stream2_))

} // namespace silkrpc

#endif  // SILKRPC_COMMON_LOG_HPP_
