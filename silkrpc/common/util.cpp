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

#include "util.hpp"

namespace silkworm {

std::ostream& operator<<(std::ostream& out, const Account& account) {
    out << "nonce: " << account.nonce;
    out << " balance: " << account.balance;
    out << " code_hash: 0x" << account.code_hash;
    out << " incarnation: " << account.incarnation;
    return out;
}

} // namespace silkworm

namespace silkrpc {

static const char* kBase64Chars[2] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/",

    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_"
};

std::string base64_encode(const uint8_t* bytes_to_encode, size_t len, bool url) {
    size_t len_encoded = (len +2) / 3 * 4;

    unsigned char trailing_char = url ? '.' : '=';

 //
 // Choose set of base64 characters. They differ
 // for the last two positions, depending on the url
 // parameter.
 // A bool (as is the parameter url) is guaranteed
 // to evaluate to either 0 or 1 in C++ therefore,
 // the correct character set is chosen by subscripting
 // base64_chars with url.
 //
    const char* base64_chars_ = kBase64Chars[url ? 1 : 0];

    std::string ret;
    ret.reserve(len_encoded);

    unsigned int pos = 0;
    while (pos < len) {
        ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);

        if (pos + 1 < len) {
           ret.push_back(base64_chars_[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);

           if (pos + 2 < len) {
              ret.push_back(base64_chars_[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
              ret.push_back(base64_chars_[  bytes_to_encode[pos + 2] & 0x3f]);
           } else {
              ret.push_back(base64_chars_[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
              ret.push_back(trailing_char);
           }
        } else {
            ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0x03) << 4]);
            ret.push_back(trailing_char);
            ret.push_back(trailing_char);
        }

        pos += 3;
    }

    return ret;
}

static const uint64_t kMask = 0x8000000000000000;

std::string to_dec(intx::uint256 number) {
    uint64_t n[4];
    for (auto idx = 0; idx < intx::uint256::num_words; idx++) {
        n[idx] = number[idx];
    }
    char s[256 / 3 + 1 + 1];
    char* p = s;
    int i;

    memset(s, '0', sizeof(s) - 1);
    s[sizeof(s) - 1] = '\0';

    auto count = 256;
    auto base = 0;
    while (n[3] == 0 && base < 4) {
        n[3] = n[2];
        n[2] = n[1];
        n[1] = n[0];

        n[base++] = 0;
        count -= 64;
    }

    for (i = 0; i < count; i++) {
        int j, carry;

        carry = (n[3] >= kMask);
        // Shift n[] left, doubling it
        n[3] = (n[3] << 1) + (n[2] >= kMask);
        n[2] = (n[2] << 1) + (n[1] >= kMask);
        n[1] = (n[1] << 1) + (n[0] >= kMask);
        n[0] = (n[0] << 1);

        // Add s[] to itself in decimal, doubling it
        for (j = sizeof(s) - 2; j >= 0; j--) {
            s[j] += s[j] - '0' + carry;

            carry = (s[j] > '9');

            if (carry) {
                s[j] -= 10;
            }
        }
    }

    while ((p[0] == '0') && (p < &s[sizeof(s) - 2])) {
        p++;
    }

    return std::string{p};
}

} // namespace silkrpc
