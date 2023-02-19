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

#include "cursor.hpp"

#include <silkrpc/common/log.hpp>

namespace silkrpc::ethdb {

SplitDupSortCursor::SplitDupSortCursor(CursorDupSort& inner_cursor, silkworm::ByteView key, silkworm::ByteView subkey, uint64_t match_bits, 
                                       uint64_t part1_end, uint64_t part2_start, uint64_t value_offset)
: inner_cursor_{inner_cursor}, key_{key}, subkey_{subkey} {
    part1_end_ = part1_end;
    part2_start_ = part2_start;
    value_offset_ = value_offset;

    match_bytes_ = (match_bits + 7) / 8;

    uint8_t shiftbits = match_bits & 7;
    if (shiftbits != 0) {
        mask_ = 0xff << (8 - shiftbits);
    } else {
        mask_ = 0xff;
    }

    first_bytes_ = key.substr(0, match_bytes_ - 1);
    if (match_bytes_ > 0) {
        last_bits_ = key[match_bytes_ - 1] & mask_;
    }
}

boost::asio::awaitable<SplittedKeyValue> SplitDupSortCursor::seek_both() {
    auto value = co_await inner_cursor_.seek_both(key_, subkey_);
    co_return split_key_value(KeyValue{key_, value});
}

boost::asio::awaitable<SplittedKeyValue> SplitDupSortCursor::next_dup() {
    KeyValue kv = co_await inner_cursor_.next_dup();
    co_return split_key_value(kv);
}

bool SplitDupSortCursor::match_key(const silkworm::ByteView& key) {
    if (key.length() == 0) {
        return false;
    }
    if (match_bytes_ == 0) {
        return true;
    }
    if (key.length() < match_bytes_) {
        return false;
    }
    if (first_bytes_ != key.substr(0, match_bytes_ - 1)) {
        return false;
    }
    return ((key[match_bytes_ - 1] & mask_) == last_bits_);
}

SplittedKeyValue SplitDupSortCursor::split_key_value(const KeyValue& kv) {
    const silkworm::Bytes& key = kv.key;

    if (key.length() == 0) {
        return SplittedKeyValue{};
    }
    if (!match_key(key)) {
        return SplittedKeyValue{};
    }

    SplittedKeyValue skv{};
    if (kv.value.size() >= value_offset_) {
       skv.key1 = key.substr(0, part1_end_);
       skv.key2 = kv.value.substr(0, value_offset_);
       skv.value =  kv.value.substr(value_offset_);
    }

    return skv;
}

} // namespace silkrpc::ethdb
