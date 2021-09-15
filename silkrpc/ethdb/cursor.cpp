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

namespace silkrpc::ethdb {

SplitCursor::SplitCursor(Cursor& inner_cursor, const silkworm::Bytes& key, uint64_t match_bits, uint64_t length1, uint64_t length2)
: inner_cursor_{inner_cursor}, key_{key} {
    length1_ = length1;
    length2_ = length2;

    match_bytes_ = (match_bits + 7) / 8;

    uint8_t shiftbits = match_bits & 7;
    if (shiftbits != 0) {
        mask_ = 0xff << (8 - shiftbits);
    } else {
        mask_ = 0xff;
    }

    first_bytes_ = key.substr(0, match_bytes_ - 1);
    last_bits_ = key[match_bytes_ - 1] & mask_;
}

asio::awaitable<SplittedKeyValue> SplitCursor::seek() {
    KeyValue kv = co_await inner_cursor_.seek(key_);
    co_return split_key_value(kv);
}

asio::awaitable<SplittedKeyValue> SplitCursor::next() {
    KeyValue kv = co_await inner_cursor_.next();
    co_return split_key_value(kv);
}

bool SplitCursor::match_key(const silkworm::ByteView& key) {
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

SplittedKeyValue SplitCursor::split_key_value(const KeyValue& kv) {
    const silkworm::Bytes& key = kv.key;

    if (key.length() == 0) {
        return SplittedKeyValue{};
    }
    if (!match_key(key)) {
        return SplittedKeyValue{};
    }

    SplittedKeyValue skv{key.substr(0, length1_)};

    if (key.length() > length1_) {
        skv.key2 = kv.key.substr(length1_, length2_);
    }
    if (key.length() > length1_ + length2_) {
        skv.key3 = kv.key.substr(length1_ + length2_);
    }

    skv.value = kv.value;

    return skv;
}

} // namespace silkrpc::ethdb
