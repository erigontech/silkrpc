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

#ifndef SILKRPC_TYPES_BLOCK_HPP_
#define SILKRPC_TYPES_BLOCK_HPP_

#include <iostream>
#include <memory>
#include <string>
#include <variant>

#include <intx/intx.hpp>

#include <silkworm/common/base.hpp>
#include <silkworm/types/block.hpp>

namespace silkrpc {

struct Block : public silkworm::BlockWithHash {
    intx::uint256 total_difficulty{0};
    bool full_tx{false};

    uint64_t get_block_size() const;
};

std::ostream& operator<<(std::ostream& out, const Block& b);

class BlockNumberOrHash {
public:
    explicit BlockNumberOrHash(std::string const& bnoh) { build(bnoh); }
    explicit BlockNumberOrHash(uint64_t number) noexcept : value_{number} {}

    virtual ~BlockNumberOrHash() noexcept {}

    BlockNumberOrHash(BlockNumberOrHash &&bnoh) = default;
    BlockNumberOrHash(BlockNumberOrHash const& bnoh) noexcept : value_{bnoh.value_} {}

    BlockNumberOrHash& operator=(BlockNumberOrHash const& bnoh) = delete;

    bool is_number() const {
        return std::holds_alternative<uint64_t>(value_);
    }

    uint64_t number() const {
        return is_number() ? *std::get_if<uint64_t>(&value_) : 0;
    }

    bool is_hash() const {
        return std::holds_alternative<evmc::bytes32>(value_);
    }

    evmc::bytes32 hash() const {
        return is_hash() ? *std::get_if<evmc::bytes32>(&value_) : evmc::bytes32{0};
    }

    bool is_tag() const {
        return std::holds_alternative<std::string>(value_);
    }

    std::string tag() const {
        return is_tag() ? *std::get_if<std::string>(&value_) : "";
    }

private:
    void build(std::string const& bnoh);

    std::variant<uint64_t, evmc::bytes32, std::string> value_;
};

std::ostream& operator<<(std::ostream& out, const BlockNumberOrHash& b);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_BLOCK_HPP_
