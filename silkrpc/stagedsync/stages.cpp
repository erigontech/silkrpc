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

#include "stages.hpp"

#include <exception>

#include <silkworm/db/tables.hpp>
#include <silkworm/rlp/decode.hpp>

namespace silkrpc::stages {

class Exception : public std::exception {
public:
    explicit Exception(const char* message) : message_{message} {};
    explicit Exception(const std::string& message) : message_{message} {};
    virtual ~Exception() noexcept {};
    const char* what() const noexcept override { return message_.c_str(); }

protected:
    std::string message_;
};

asio::awaitable<uint64_t> get_sync_stage_progress(kv::Database& database, const Bytes& stage_key) {
    const auto kv_pair = co_await database.begin()->cursor()->seek(silkworm::db::table::kSyncStageProgress.name, stage_key);
    const auto key = kv_pair.key;
    if (key != stage_key) {
        throw Exception("stage key mismatch, expected " + to_hex(stage_key) + " got " + to_hex(key));
    }

    const auto value = kv_pair.value;
    if (value.length() == 0) {
        co_return 0;
    }
    if (value.length() < 8) {
        throw Exception("data too short, expected 8 got " + std::to_string(value.length()));
    }
    ByteView data{value.substr(0, 8)};
    uint64_t block_height = rlp::read_uint64(data, /*allow_leading_zeros=*/true);

    co_return block_height;
}

} // namespace silkrpc::stages
