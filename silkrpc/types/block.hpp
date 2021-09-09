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
    BlockNumberOrHash() {}
    BlockNumberOrHash(BlockNumberOrHash const& bnoh);
    explicit BlockNumberOrHash(std::string const& bnoh) {
        build(bnoh);
    }
    explicit BlockNumberOrHash(std::uint64_t const& number)
        : number_{std::make_unique<std::uint64_t>(number)} {};

    virtual ~BlockNumberOrHash() noexcept {}

    BlockNumberOrHash& operator=(BlockNumberOrHash const& bnoh) = delete;
    BlockNumberOrHash& operator=(std::string const& bnoh)  {
        build(bnoh);
        return *this;
    }

    BlockNumberOrHash& operator=(std::uint64_t const number) {
        tag_.release();
        hash_.release();
        number_ = std::make_unique<std::uint64_t>(number);
        return *this;
    }

    bool is_undefined() const {
        return !(is_number() || is_tag() || is_hash());
    }

    bool is_number() const {
        return number_.get() != nullptr;
    }

    uint64_t number() const {
        return is_number() ? *number_ : 0;
    }

    bool is_hash() const {
        return hash_.get() != nullptr;
    }

    evmc::bytes32 hash() const {
        return is_hash() ? *hash_ : evmc::bytes32{0};
    }

    bool is_tag() const {
        return tag_.get() != nullptr;
    }

    std::string tag() const {
        return is_tag() ? *tag_ : "";
    }

private:
    void build(std::string const& bnoh);
    void set_number(std::string const& input, int base);

    std::unique_ptr<std::uint64_t> number_;
    std::unique_ptr<evmc::bytes32> hash_;
    std::unique_ptr<std::string> tag_;
};

std::ostream& operator<<(std::ostream& out, const BlockNumberOrHash& b);

} // namespace silkrpc

#endif  // SILKRPC_TYPES_BLOCK_HPP_
