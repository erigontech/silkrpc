/*
   Copyright 2020-2022 The Silkrpc Authors

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

#include <array>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <benchmark/benchmark.h>
#include <intx/int128.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>
#include <silkworm/common/endian.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/types/bloom.hpp>

#include <silkrpc/common/util.hpp>
#include <silkrpc/json/lithium.hpp>
#include <silkrpc/json/lithium_json.hpp>
#include <silkrpc/json/lithium_symbol.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/transaction.hpp>

#define MAX_THRS 6
#define MAX_ENCODE 1000000
pthread_t tid[MAX_THRS];

using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

static constexpr int kHexAddressSize = 2 + 20 * 2; // 0x + 2 bytes for each hex digit
static constexpr int kQuotedHexAddressSize = 2 + kHexAddressSize; // ""
static constexpr int kHexHashSize = 2 + 32 * 2; // 0x + 2 bytes for each hex digit
static constexpr int kQuotedHexHashSize = 2 + kHexHashSize; // ""
static constexpr int kHexBloomSize = 2 + silkworm::kBloomByteLength * 2; // 0x + 2 bytes for each hex digit
static constexpr int kHexNonceSize = 2 + 8 * 2; // 0x + 2 bytes for each hex digit
static constexpr int kHexUint256Size = 2 + 32 * 2; // 0x + 2 bytes for each hex digit
static constexpr int kHexBytes32Size = 2 + 32 * 2; // 0x + 2 bytes for each hex digit

static constexpr evmc::address ADDR{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address};
static constexpr evmc::bytes32 HASH{0x3ac225168df54212a25c1c01fd35bebfea408fdac2e31ddd6f80a4bbf9a5f1cb_bytes32};

static const std::string ADDR_STRING{"\"0x0715a7794a1dc8e42615f059dd6e406a6594651a\""};
static const std::string HASH_STRING{"\"0x3ac225168df54212a25c1c01fd35bebfea408fdac2e31ddd6f80a4bbf9a5f1cb\""};

// ASCII -> hex value (0xbc means bad [hex] char)
static constexpr uint8_t kUnhexTable[256] = {
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc};

// ASCII -> hex value << 4 (upper nibble) (0xbc means bad [hex] char)
static constexpr uint8_t kUnhexTable4[256] = {
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
    0x90, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
    0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc};

static inline uint8_t unhex_lut(uint8_t x) { return kUnhexTable[x]; }
static inline uint8_t unhex_lut4(uint8_t x) { return kUnhexTable4[x]; }

template<typename T, std::size_t N>
std::ostream& operator<<(std::ostream& o, const std::array<T, N>& a) {
    std::copy(a.cbegin(), a.cend(), std::ostream_iterator<T>(o, ""));
    return o;
}


template<std::size_t N>
void to_hex(std::array<char, 2 + N * 2>& hex_bytes, const uint8_t bytes[N]) {
    static const char* kHexDigits{"0123456789abcdef"};
    hex_bytes[0] = '0';
    hex_bytes[1] = 'x';
    for (auto i{0}, j{2}; i<N; i++, j+=2) {
        const auto v = bytes[i];
        hex_bytes[j] = kHexDigits[v >> 4];
        hex_bytes[j+1] = kHexDigits[v & 0x0f];
    }
}

constexpr auto nonce_to_hex = to_hex<8>;
constexpr auto address_to_hex = to_hex<20>;
constexpr auto bytes32_to_hex = to_hex<32>;
constexpr auto bloom_to_hex = to_hex<256>;

size_t to_hex(char *hex_bytes, silkworm::ByteView bytes) {
    static const char* kHexDigits{"0123456789abcdef"};
    char* dest{&hex_bytes[0]};
    *dest++ = '0';
    *dest++ = 'x';
    for (const auto& b : bytes) {
        *dest++ = kHexDigits[b >> 4];    // Hi
        *dest++ = kHexDigits[b & 0x0f];  // Lo
    }
    return dest-hex_bytes;
}


template<std::size_t N>
std::size_t to_my_hex(char *hex_bytes, const uint8_t bytes[N]) {
    static const char* kHexDigits{"0123456789abcdef"};
    char* dest{&hex_bytes[0]};
    *dest++ = '0';
    *dest++ = 'x';
    for (int i = 0; i<N; i++) {
        const auto v = bytes[i];
        *dest++ = kHexDigits[v >> 4];    // Hi
        *dest++ = kHexDigits[v & 0x0f];  // Lo
    }
    return dest-hex_bytes;
}
constexpr auto address_to_hex2 = to_my_hex<20>;
constexpr auto bytes32_to_hex2 = to_my_hex<32>;
constexpr auto nonce_to_hex2 = to_my_hex<8>;
constexpr auto bloom_to_hex2 = to_my_hex<256>;


template<std::size_t N>
bool from_hex(uint8_t (&bytes)[N], std::string_view& hex) {
    if (hex.size() >= 2 && hex[0] != '0' || (hex[1] != 'x' && hex[1] != 'X')) {
        return false;
    }

    const char* src{hex.data() + 2};
    uint8_t* dst{bytes};
    std::size_t pos = N & 1;
    if (pos) {
        const auto b{unhex_lut(static_cast<uint8_t>(*src++))};
        if (b == 0xbc) {
            return false;
        }
        *dst++ = b;
    }

    for (; pos < N; ++pos) {
        const auto a{unhex_lut4(static_cast<uint8_t>(*src++))};
        const auto b{unhex_lut(static_cast<uint8_t>(*src++))};
        if (a == 0xbc || b == 0xbc) {
            return false;
        }
        *dst++ = a | b;
    }

    return true;
}

constexpr auto address_from_hex = from_hex<20>;
constexpr auto bytes32_from_hex = from_hex<32>;

void ensure(bool condition, const std::optional<std::string> message = std::nullopt) {
    if (!condition) {
        throw std::logic_error("ensure: " + (message ? *message : "CHECK failed"));
    }
}

/* benchmark_encode_address */
static void benchmark_encode_address_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = ADDR;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
    }
}

static void benchmark_encode_address_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        char buffer[kQuotedHexAddressSize];
        li::output_buffer output_buffer{buffer, kQuotedHexAddressSize};
        std::array<char, kHexAddressSize> hex_bytes;
        address_to_hex(hex_bytes, ADDR.bytes);
        li::json_encode(output_buffer, std::string_view{hex_bytes.data(), hex_bytes.size()});
        const auto s = output_buffer.to_string_view();
        benchmark::DoNotOptimize(s);
        output_buffer.reset();
    }
}

/* benchmark_decode_address */
static void benchmark_decode_address_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const auto address_json = nlohmann::json::parse(ADDR_STRING);
        const auto address = address_json.get<evmc::address>();
        benchmark::DoNotOptimize(address);
    }
}

static void benchmark_decode_address_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        std::string_view hex;
        li::json_decode(ADDR_STRING, hex);
        evmc::address address;
        address_from_hex(address.bytes, hex);
    }
}

/* benchmark_encode_bytes32 */
static void benchmark_encode_bytes32_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = HASH;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
    }
}

void encode_bytes32(li::output_buffer& output_buffer, const evmc::bytes32& b32) {
    std::array<char, kHexHashSize> hex_bytes;
    bytes32_to_hex(hex_bytes, b32.bytes);
    li::json_encode(output_buffer, std::string_view{hex_bytes.data(), hex_bytes.size()});
}

static void benchmark_encode_bytes32_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        char buffer[kQuotedHexHashSize];
        li::output_buffer output_buffer{buffer, kQuotedHexHashSize};
        encode_bytes32(output_buffer, HASH);
        output_buffer.reset();
    }
}

/* benchmark_decode_bytes32 */
static void benchmark_decode_bytes32_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const auto bytes32_json = nlohmann::json::parse(HASH_STRING);
        const auto hash = bytes32_json.get<evmc::bytes32>();
        benchmark::DoNotOptimize(hash);
    }
}

static void benchmark_decode_bytes32_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        std::string_view hex;
        li::json_decode(HASH_STRING, hex);
        evmc::bytes32 hash;
        bytes32_from_hex(hash.bytes, hex);
    }
}

/* benchmark_decode_uint256 */
static const std::string UINT256_STRING{"\"0x752f02b1438be7f67ebf0e71310db3514b162fb169cdb95ad15dde38eff7719b\""};

static void benchmark_decode_uint256_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const auto uint256_json = nlohmann::json::parse(UINT256_STRING);
        const auto i = uint256_json.get<intx::uint256>();
        benchmark::DoNotOptimize(i);
    }
}

// TODO: this could be added to evmone
namespace intx {
template <typename Int>
inline constexpr Int from_string(std::string_view s)
{
    auto x = Int{};
    int num_digits = 0;

    if (s[0] == '0' && s[1] == 'x')
    {
        s.remove_prefix(2);
        while (s.size() > 0)
        {
            const auto c = s[0];
            s.remove_prefix(1);
            if (++num_digits > int{sizeof(x) * 2})
                intx::throw_<std::out_of_range>(s.data());
            x = (x << uint64_t{4}) | intx::from_hex_digit(c);
        }
        return x;
    }

    while (s.size() > 0)
    {
        const auto c = s[0];
        s.remove_prefix(1);
        if (num_digits++ > std::numeric_limits<Int>::digits10)
            intx::throw_<std::out_of_range>(s.data());

        const auto d = intx::from_dec_digit(c);
        x = x * Int{10} + d;
        if (x < d)
            intx::throw_<std::out_of_range>(s.data());
    }
    return x;
}
}

static void benchmark_decode_uint256_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        std::string_view hex;
        li::json_decode(UINT256_STRING, hex);
        const auto i = intx::from_string<intx::uint256>(hex);
        benchmark::DoNotOptimize(i);
    }
}

#ifndef LI_SYMBOL_transactions_root
#define LI_SYMBOL_transactions_root
    LI_SYMBOL(transactions_root)
#endif // LI_SYMBOL_transactions_root

#ifndef LI_SYMBOL_state_root
#define LI_SYMBOL_state_root
    LI_SYMBOL(state_root)
#endif // LI_SYMBOL_state_root

#ifndef LI_SYMBOL_receipts_root
#define LI_SYMBOL_receipts_root
    LI_SYMBOL(receipts_root)
#endif // LI_SYMBOL_receipts_root

#ifndef LI_SYMBOL_beneficiary
#define LI_SYMBOL_beneficiary
    LI_SYMBOL(beneficiary)
#endif // LI_SYMBOL_beneficiary

#ifndef LI_SYMBOL_mix_hash
#define LI_SYMBOL_mix_hash
    LI_SYMBOL(mix_hash)
#endif // LI_SYMBOL_mix_hash

#ifndef LI_SYMBOL_logs_bloom
#define LI_SYMBOL_logs_bloom
    LI_SYMBOL(logs_bloom)
#endif // LI_SYMBOL_logs_bloom

#ifndef LI_SYMBOL_nonce
#define LI_SYMBOL_nonce
    LI_SYMBOL(nonce)
#endif // LI_SYMBOL_nonce

#ifndef LI_SYMBOL_difficulty
#define LI_SYMBOL_difficulty
    LI_SYMBOL(difficulty)
#endif // LI_SYMBOL_difficulty

#ifndef LI_SYMBOL_number
#define LI_SYMBOL_number
    LI_SYMBOL(number)
#endif // LI_SYMBOL_number

#ifndef LI_SYMBOL_gas_limit
#define LI_SYMBOL_gas_limit
    LI_SYMBOL(gas_limit)
#endif // LI_SYMBOL_gas_limit

#ifndef LI_SYMBOL_gas_used
#define LI_SYMBOL_gas_used
    LI_SYMBOL(gas_used)
#endif // LI_SYMBOL_gas_used

#ifndef LI_SYMBOL_timestamp
#define LI_SYMBOL_timestamp
    LI_SYMBOL(timestamp)
#endif // LI_SYMBOL_timestamp

#ifndef LI_SYMBOL_extra_data
#define LI_SYMBOL_extra_data
    LI_SYMBOL(extra_data)
#endif // LI_SYMBOL_extra_data

#ifndef LI_SYMBOL_get_block_number
#define LI_SYMBOL_get_block_number
    LI_SYMBOL(get_block_number)
#endif // LI_SYMBOL_get_block_number

#ifndef LI_SYMBOL_get_gas_used
#define LI_SYMBOL_get_gas_used
    LI_SYMBOL(get_gas_used)
#endif // LI_SYMBOL_get_gas_used

#ifndef LI_SYMBOL_get_timestamp
#define LI_SYMBOL_get_timestamp
    LI_SYMBOL(get_timestamp)
#endif // LI_SYMBOL_get_timestamp

#ifndef LI_SYMBOL_get_gas_limit
#define LI_SYMBOL_get_gas_limit
    LI_SYMBOL(get_gas_limit)
#endif // LI_SYMBOL_get_gas_limit

#ifndef LI_SYMBOL_get_value
#define LI_SYMBOL_get_value
    LI_SYMBOL(get_value)
#endif // LI_SYMBOL_get_value

#ifndef LI_SYMBOL_get_v
#define LI_SYMBOL_get_v
    LI_SYMBOL(get_v)
#endif // LI_SYMBOL_get_v

#ifndef LI_SYMBOL_get_r
#define LI_SYMBOL_get_r
    LI_SYMBOL(get_r)
#endif // LI_SYMBOL_get_r

#ifndef LI_SYMBOL_get_s
#define LI_SYMBOL_get_s
    LI_SYMBOL(get_s)
#endif // LI_SYMBOL_get_s


template<std::size_t N>
std::size_t to_hex_no_leading_zeros(std::array<char, N>& hex_bytes, silkworm::ByteView bytes) {
    static const char* kHexDigits{"0123456789abcdef"};

    std::size_t position = 0;
    hex_bytes[position++] = '0';
    hex_bytes[position++] = 'x';

    bool found_nonzero{false};
    for (size_t i{0}; i < bytes.length(); ++i) {
        uint8_t x{bytes[i]};
        char lo{kHexDigits[x & 0x0f]};
        char hi{kHexDigits[x >> 4]};
        if (!found_nonzero && hi != '0') {
            found_nonzero = true;
        }
        if (found_nonzero) {
            hex_bytes[position++] = hi;
        }
        if (!found_nonzero && lo != '0') {
            found_nonzero = true;
        }
        if (found_nonzero || i == bytes.length() - 1) {
            hex_bytes[position++] = lo;
        }
    }

    return position;
}

std::size_t to_hex_no_leading_zeros(char *hex_bytes, silkworm::ByteView bytes) {
    static const char* kHexDigits{"0123456789abcdef"};

    char* dest{&hex_bytes[0]};
    int len = bytes.length();
    *dest++ = '0';
    *dest++ = 'x';

    bool found_nonzero{false};
    for (size_t i{0}; i < len; ++i) {
        uint8_t x{bytes[i]};
        char lo{kHexDigits[x & 0x0f]};
        char hi{kHexDigits[x >> 4]};
        if (!found_nonzero && hi != '0') {
            found_nonzero = true;
        }
        if (found_nonzero) {
            *dest++ = hi;
        }
        if (!found_nonzero && lo != '0') {
            found_nonzero = true;
        }
        if (found_nonzero || i == len - 1) {
            *dest++ = lo;
        }
    }

    return dest-hex_bytes;
}

/*std::string to_hex_no_leading_zeros(uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    silkworm::endian::store_big_u64(&number_bytes[0], number);
    return to_hex_no_leading_zeros(number_bytes);
}*/

std::size_t to_quantity(char *quantity_hex_bytes, silkworm::ByteView bytes) {
    return to_hex_no_leading_zeros(quantity_hex_bytes, bytes);
}

template<std::size_t N>
std::size_t to_quantity(std::array<char, N>& quantity_hex_bytes, silkworm::ByteView bytes) {
    return to_hex_no_leading_zeros<N>(quantity_hex_bytes, bytes);
}

std::size_t to_quantity(char *quantity_hex_bytes, uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    silkworm::endian::store_big_u64(number_bytes.data(), number);
    return to_hex_no_leading_zeros(quantity_hex_bytes, number_bytes);
}

std::size_t to_quantity(std::array<char, 8>& quantity_hex_bytes, uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    silkworm::endian::store_big_u64(number_bytes.data(), number);
    return to_hex_no_leading_zeros<8>(quantity_hex_bytes, number_bytes);
}

std::size_t to_quantity(char *quantity_hex_bytes, intx::uint256 number) {
    if (number == 0) {
       quantity_hex_bytes[0] = '0';
       quantity_hex_bytes[1] = 'x';
       quantity_hex_bytes[2] = '0';
       return 3;
    }
    return to_quantity(quantity_hex_bytes, silkworm::endian::to_big_compact(number));
}


std::size_t to_quantity(std::array<char, 8>& quantity_hex_bytes, intx::uint256 number) {
    if (number == 0) {
       quantity_hex_bytes[0] = '0';
       quantity_hex_bytes[1] = 'x';
       quantity_hex_bytes[2] = '0';
       return 3;
    }
    return to_quantity(quantity_hex_bytes, silkworm::endian::to_big_compact(number));
}

struct BlockHeader : public silkworm::BlockHeader {
   std::array<char, 8> block_number_quantity;
   std::array<char, 8> gas_used_quantity;
   std::array<char, 8> gas_limit_quantity;
   std::array<char, 8> timestamp_quantity;

   std::string_view get_block_number() { 
          auto block_number_quantity_size = to_quantity(block_number_quantity, number);
          return std::string_view{block_number_quantity.data(), block_number_quantity_size }; 
   }

   std::string_view get_gas_used() { 
          auto gas_used_quantity_size = to_quantity(gas_used_quantity, gas_used);
          return std::string_view{gas_used_quantity.data(), gas_used_quantity_size }; 
   }

   std::string_view get_gas_limit() { 
          auto gas_limit_quantity_size = to_quantity(gas_limit_quantity, gas_limit);
          return std::string_view{gas_limit_quantity.data(), gas_limit_quantity_size }; 
   }

   std::string_view get_timestamp() { 
          auto timestamp_quantity_size = to_quantity(timestamp_quantity, timestamp);
          return std::string_view{timestamp_quantity.data(), timestamp_quantity_size }; 
   }

};

struct Transaction : public silkworm::Transaction {
   std::array<char, 8> gas_limit_quantity;
   std::array<char, 8> type_quantity;
   std::array<char, 8> nonce_quantity;
   std::array<char, 8> value_quantity;
   std::array<char, 64+2> v_quantity;
   std::array<char, 64+2> s_quantity;
   std::array<char, 64+2> r_quantity;

   std::string_view get_gas_limit() { 
          auto gas_limit_quantity_size = to_quantity(gas_limit_quantity, gas_limit);
          return std::string_view{gas_limit_quantity.data(), gas_limit_quantity_size }; 
   }

   std::string_view get_type() { 
          auto type_quantity_size = to_quantity(type_quantity, (uint8_t)type);
          return std::string_view{type_quantity.data(), type_quantity_size }; 
   }

   std::string_view get_nonce() { 
          auto nonce_quantity_size = to_quantity(nonce_quantity, nonce);
          return std::string_view{nonce_quantity.data(), nonce_quantity_size }; 
   }

   std::string_view get_value() { 
          auto value_quantity_size = to_quantity(value_quantity, value);
          return std::string_view{value_quantity.data(), value_quantity_size }; 
   }

   std::string_view get_v() { 
          auto v_quantity_size = to_quantity(v_quantity, silkworm::endian::to_big_compact(v()));
          return std::string_view{v_quantity.data(), v_quantity_size }; 
   }

   std::string_view get_r() { 
          auto r_quantity_size = to_quantity(r_quantity, silkworm::endian::to_big_compact(r));
          return std::string_view{r_quantity.data(), r_quantity_size }; 
   }

   std::string_view get_s() { 
          auto s_quantity_size = to_quantity(s_quantity, silkworm::endian::to_big_compact(s));
          return std::string_view{s_quantity.data(), s_quantity_size }; 
   }

   std::string_view get_hash() { 
          std::array<char, kHexHashSize> hex_bytes;
          auto ethash_hash{hash_of_transaction(*this)};
          evmc::bytes32 hash = silkworm::to_bytes32({ethash_hash.bytes, silkworm::kHashLength});
          bytes32_to_hex(hex_bytes, hash.bytes);
          return std::string_view{hex_bytes.data(), hex_bytes.size()};
   }
};

struct BlockBody  {
    std::vector<Transaction> transactions;
    std::vector<BlockHeader> ommers;
};

struct Block : public BlockBody {
    BlockHeader header;
    evmc::bytes32 hash;
    intx::uint256 total_difficulty{0};
    bool full_tx;
    int tmp{123}; // PXB

    std::array<char, 8> block_size_quantity;


    std::string_view get_block_size() { 
          auto block_size_quantity_size = to_quantity(block_size_quantity, tmp); 
          return std::string_view{block_size_quantity.data(), block_size_quantity_size }; 
    }
};

/* benchmark_encode_block_header */
static const BlockHeader HEADER{
    0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
    0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d_bytes32,
    0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
    0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d_bytes32,
    0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e_bytes32,
    0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f_bytes32,
    silkworm::Bloom{},
    intx::uint256{0},
    uint64_t(5),
    uint64_t(1000000),
    uint64_t(1000000),
    uint64_t(5405021),
    *silkworm::from_hex("0001FF0100"),                                          // extradata
    0x0000000000000000000000000000000000000000000000000000000000000001_bytes32, // mixhash
    {1, 2, 3, 4, 5, 6, 7, 8},                                                   // nonce
    std::optional<intx::uint256>(1000),                                         // base_fee_per_gas
    "",
    "",
    "",
    ""
};

static const BlockHeader OMMER_HEADER{
    0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
    0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d_bytes32,
    0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,
    0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d_bytes32,
    0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e_bytes32,
    0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f_bytes32,
    silkworm::Bloom{},
    intx::uint256{0},
    uint64_t(5),
    uint64_t(1000000),
    uint64_t(1000000),
    uint64_t(5405021),
    *silkworm::from_hex("0001FF0100"),                                          // extradata
    0x0000000000000000000000000000000000000000000000000000000000000001_bytes32, // mixhash
    {1, 2, 3, 4, 5, 6, 7, 8},                                                   // nonce
    std::optional<intx::uint256>(1000),                                         // base_fee_per_gas
    "",
    "",
    "",
    ""
};

static const Transaction TRANSACTION_LEGACY{
    silkworm::Transaction::Type::kLegacy,                // type
    0,                                                   // nonce
    50'000 * silkworm::kGiga,                            // max_priority_fee_per_gas
    50'000 * silkworm::kGiga,                            // max_fee_per_gas
    uint64_t(18),                                        // gas_limit
    0x5df9b87991262f6ba471f09758cde1c0fc1de734_address,  // to
    31337,                                               // value
    {},                                                  // data
    true,                                                // odd_y_parity
    std::nullopt,                                        // chain_id
    intx::from_string<intx::uint256>("0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0"),  // r
    intx::from_string<intx::uint256>("0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a"),  // s
    {},                                                  // access_list
    0x6df9b87991262f6ba471f09758cde1c0fc1de734_address,  // from
};

static const Transaction TRANSACTION_EIP2930{
    silkworm::Transaction::Type::kEip2930,               // type
    0,                                                   // nonce
    20000000000,                                         // max_priority_fee_per_gas
    20000000000,                                         // max_fee_per_gas
    uint64_t(0),                                         // gas_limit
    0x0715a7794a1dc8e42615f059dd6e406a6594651a_address,  // to
    intx::uint256{0},                                    // value
    {},                                                  // data
    false,                                               // odd_y_parity
    std::nullopt,                                        // chain_id
    intx::uint256{1},                                    // r
    intx::uint256{18},                                   // s
    {},                                                  // access_list
    0x007fb8417eb9ad4d958b050fc3720d5b46a2c053_address   // from
};

static const Block BLOCK1{
    std::vector<Transaction>{TRANSACTION_LEGACY, TRANSACTION_EIP2930},
    std::vector<BlockHeader>{OMMER_HEADER},
    HEADER,
    0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
    true,
    0,                                                                           // total_difficulty
    false                                                                        // full_tx
};

static const silkrpc::Block BLOCK{
    std::vector<silkworm::Transaction>{TRANSACTION_LEGACY, TRANSACTION_EIP2930},
    std::vector<silkworm::BlockHeader>{OMMER_HEADER},
    HEADER,
    0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
    4,                                                                           // total_difficulty
    true                                                                         // full_tx
};

static const silkrpc::Block BIG_BLOCK{
    std::vector<silkworm::Transaction>{TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930, TRANSACTION_LEGACY, TRANSACTION_EIP2930},
    std::vector<silkworm::BlockHeader>{OMMER_HEADER},
    HEADER,
    0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32,
    4,                                                                           // total_difficulty
    true                                                                         // full_tx
};

static void benchmark_encode_transaction_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = TRANSACTION_LEGACY;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
        //std::cout << "nlohmann::transaction: "  << s << "\n";
    }
}
BENCHMARK(benchmark_encode_transaction_nlohmann_json);

static void benchmark_encode_block_header_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = HEADER;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
        //std::cout << "nlohmann::block_header: "  << s << "\n";
    }
}
BENCHMARK(benchmark_encode_block_header_nlohmann_json);


static void benchmark_encode_block_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = BLOCK;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
        //std::cout << "nlohmann::block: "  << s << "\n";
    }
}
BENCHMARK(benchmark_encode_block_nlohmann_json);

void *encode_nlohmann_child(void *arg) {
    int i;
    for (i = 0; i < MAX_ENCODE; i++) {
        const nlohmann::json j = BLOCK; // BIG_BLOCK
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
    }
    return 0;
}

void encode_nlohmann() {
    int i;

    for (i = 0; i < MAX_THRS; i++) {
        pthread_create(&tid[i], NULL, &encode_nlohmann_child, NULL);
    }
    for (i = 0; i < MAX_THRS; i++) {
        pthread_join(tid[i], NULL);
    }
}

static void benchmark_encode_block_nlohmann_batch_json(benchmark::State& state) {
    for (auto _ : state) {
       encode_nlohmann();
    }
}
BENCHMARK(benchmark_encode_block_nlohmann_batch_json);

#ifndef LI_SYMBOL_parent_hash
#define LI_SYMBOL_parent_hash
    LI_SYMBOL(parent_hash)
#endif // LI_SYMBOL_parent_hash

#ifndef LI_SYMBOL_ommers_hash
#define LI_SYMBOL_ommers_hash
    LI_SYMBOL(ommers_hash)
#endif // LI_SYMBOL_ommers_hash

#ifndef LI_SYMBOL_get_type
#define LI_SYMBOL_get_type
    LI_SYMBOL(get_type)
#endif // LI_SYMBOL_get_type

#ifndef LI_SYMBOL_max_fee_per_gas
#define LI_SYMBOL_max_fee_per_gas
    LI_SYMBOL(max_fee_per_gas)
#endif // LI_SYMBOL_max_fee_per_gas

#ifndef LI_SYMBOL_to
#define LI_SYMBOL_to
    LI_SYMBOL(to)
#endif // LI_SYMBOL_to

#ifndef LI_SYMBOL_from
#define LI_SYMBOL_from
    LI_SYMBOL(from)
#endif // LI_SYMBOL_from

#ifndef LI_SYMBOL_get_nonce
#define LI_SYMBOL_get_nonce
    LI_SYMBOL(get_nonce)
#endif // LI_SYMBOL_get_nonce

#ifndef LI_SYMBOL_data
#define LI_SYMBOL_data
    LI_SYMBOL(data)
#endif // LI_SYMBOL_data

#ifndef LI_SYMBOL_get_hash
#define LI_SYMBOL_get_hash
    LI_SYMBOL(get_hash)
#endif // LI_SYMBOL_get_hash

#ifndef LI_SYMBOL_ommers
#define LI_SYMBOL_ommers
    LI_SYMBOL(ommers)
#endif // LI_SYMBOL_ommers

#ifndef LI_SYMBOL_transactions
#define LI_SYMBOL_transactions
    LI_SYMBOL(transactions)
#endif // LI_SYMBOL_transactions

#ifndef LI_SYMBOL_header
#define LI_SYMBOL_header
    LI_SYMBOL(header)
#endif // LI_SYMBOL_header

#ifndef LI_SYMBOL_hash
#define LI_SYMBOL_hash
    LI_SYMBOL(hash)
#endif // LI_SYMBOL_hash

#ifndef LI_SYMBOL_total_difficulty
#define LI_SYMBOL_total_difficulty
    LI_SYMBOL(total_difficulty)
#endif // LI_SYMBOL_total_difficulty

#ifndef LI_SYMBOL_get_block_size
#define LI_SYMBOL_get_block_size
    LI_SYMBOL(get_block_size)
#endif // LI_SYMBOL_get_block_size



namespace li {

inline output_buffer& operator<<(output_buffer& out, const evmc::address& address) {
    std::array<char, kHexAddressSize> hex_bytes;
    address_to_hex(hex_bytes, address.bytes);
    //std::cout << "operator<< evmc::address: " << std::string_view{hex_bytes.data(), hex_bytes.size()} << "\n";
    json_encode(out, std::string_view{hex_bytes.data(), hex_bytes.size()});
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const evmc::bytes32& b32) {
    std::array<char, kHexHashSize> hex_bytes;
    bytes32_to_hex(hex_bytes, b32.bytes);
    //std::cout << "operator<<evmc::bytes32: " << std::string_view{hex_bytes.data(), hex_bytes.size()} << "\n";
    json_encode(out, std::string_view{hex_bytes.data(), hex_bytes.size()});
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const intx::uint256& u256) {
    std::array<char, kHexUint256Size> hex_bytes;
    //std::cout << "operator<<intx::uint256: u256=" << intx::to_string(u256) << "\n";
    const auto quantity_bytes_length = to_quantity(hex_bytes, silkworm::endian::to_big_compact(u256));
    //std::cout << "operator<<intx::uint256: " << std::string_view{hex_bytes.data(), quantity_bytes_length} << "\n";
    json_encode(out, std::string_view{hex_bytes.data(), quantity_bytes_length});
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const silkworm::Bloom& bloom) {
    std::array<char, kHexBloomSize> hex_bytes;
    bloom_to_hex(hex_bytes, bloom.data());
    //std::cout << "operator<<silkworm::Bloom: " << std::string_view{hex_bytes.data(), hex_bytes.size()} << "\n";
    json_encode(out, std::string_view{hex_bytes.data(), hex_bytes.size()});
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const silkworm::BlockHeader::NonceType& nonce) {
    std::array<char, kHexNonceSize> hex_bytes;
    nonce_to_hex(hex_bytes, nonce.data());
    //std::cout << "operator<<silkworm::BlockHeader::NonceType: " << std::string_view{hex_bytes.data(), hex_bytes.size()} << "\n";
    json_encode(out, std::string_view{hex_bytes.data(), hex_bytes.size()});
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const silkworm::Bytes& bytes) {
    const auto hex_bytes = "0x" + silkworm::to_hex(bytes);
    //std::cout << "operator<<silkworm::Bytes: " << std::string_view{hex_bytes.data(), hex_bytes.size()} << "\n";
    json_encode(out, std::string_view{hex_bytes.data(), hex_bytes.size()});
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const BlockHeader& block_header) {
    li::json_object(
        s::parent_hash(li::json_key("parentHash")),
        s::ommers_hash(li::json_key("sha3Uncles")),
        s::beneficiary(li::json_key("miner")),
        s::state_root(li::json_key("stateRoot")),
        s::transactions_root(li::json_key("transactionsRoot")),
        s::receipts_root(li::json_key("receiptsRoot")),
        s::logs_bloom(li::json_key("logsBloom")),
        s::difficulty,
        s::get_block_number(li::json_key("number")),
        s::get_gas_limit(li::json_key("gas_limit")),
        s::get_gas_used(li::json_key("gas_used")),
        s::get_timestamp(li::json_key("timestamp")),
        s::extra_data(li::json_key("extraData")),
        s::mix_hash(li::json_key("mixHash")),
        s::nonce).encode(out, block_header);
    return out;
}

inline output_buffer& operator<<(output_buffer& out, const Transaction& transaction) {
    li::json_object(
        s::from(li::json_key("from")),
        s::get_gas_limit(li::json_key("gas")),
        //s::get_hash(li::json_key("hash")),
        s::data(li::json_key("input")),
        s::get_nonce(li::json_key("nonce")),
        s::get_r(li::json_key("r")),
        s::get_s(li::json_key("s")),
        s::to(li::json_key("to")),
        s::get_type(li::json_key("type")),
        s::get_v(li::json_key("v")),
        //s::max_fee_per_gas,
        s::get_value(li::json_key("value"))).encode(out, transaction);
    return out;
}

#define START_OBJECT(out) \
    json_buffer& local_out = out; \
    int first_attribute = 1; \
    int first_element = 1; \
    char *start = out.buffer_; \
    char *ptr = out.curr_; 

#define END_OBJECT() local_out.set_curr(ptr);
      

#define GET_CURR_BUFF_ADDR() (ptr)

#define DUMP_JSON() \
       int tmp = ptr-start; \
       printf ("Len %d\n",tmp); \
       for (int i = 0; i < tmp; i++) \
          printf ("%c",start[i]); \
       printf("\n");
        
#define ADD_ATTRIBUTE(name, value) \
    if (first_attribute) { \
       first_attribute = 0; \
       ptr += std::sprintf (ptr, "\"%s\":\"%s\"", name,value); \
    } \
    else { \
       ptr += std::sprintf (ptr, ",\"%s\":\"%s\"", name,value); \
    }

#define START_VECTOR(name) \
       *ptr++ = ','; \
       *ptr++ = '\"'; \
       memcpy(ptr, name, strlen(name)); \
       ptr+=strlen(name); \
       *ptr++ = '\"'; \
       *ptr++ = ':'; \
       *ptr++ = '['; \
       first_element = 1; 

#define END_VECTOR() \
       *ptr++ = ']'; \

#define RESTORE() \
          ptr = local_out.curr_; 

#define SAVE() \
          local_out.set_curr(ptr); \
          *ptr++ = ','; 

#define START_ELEMENT(name) \
       if (first_element) { \
          first_element = 0; \
       } \
       else \
          *ptr++ = ','; \
       *ptr++ = '{'; \
       local_out.set_curr(ptr); 

#define END_ELEMENT(name) \
       *ptr++ = '}';

#define ADD_ATTRIBUTE_ZEROCOPY(name, len) \
    if (first_attribute) { \
       first_attribute = 0; \
       *ptr++ = '\"'; \
       memcpy(ptr, name, strlen(name)); \
       ptr+=strlen(name); \
       *ptr++ = '\"'; \
       *ptr++ = ':'; \
       *ptr++ = '\"'; \
       ptr+=len; \
       *ptr++ = '\"'; \
    } \
    else { \
       *ptr++ = ','; \
       *ptr++ = '\"'; \
       memcpy(ptr, name, strlen(name)); \
       ptr += strlen(name); \
       *ptr++ = '\"'; \
       *ptr++ = ':'; \
       *ptr++ = '\"'; \
       ptr += len; \
       *ptr++ = '\"'; \
    }
    

struct json_buffer {

  json_buffer(json_buffer&& o) : buffer_(o.buffer_), curr_(o.curr_) {
  }

  json_buffer(char *buffer, int max_len) : buffer_(buffer), curr_(buffer), max_len_(max_len) {
     *curr_++ = '{'; 
  }

  inline void set_curr(char *curr) {
     curr_=curr;
  }

  inline void reset() {
      curr_ = buffer_;
  }

  inline void end() {
     *curr_++ = '}'; 
  }

  inline void add_attribute_name(const char *name) {
    if (first_attribute) { 
       first_attribute = 0; 
       *curr_++ = '\"'; 
       memcpy(curr_, name, strlen(name)); 
       curr_+=strlen(name); 
       *curr_++ = '\"'; 
       *curr_++ = ':'; 
       *curr_++ = '\"'; \
    } 
    else { \
       *curr_++ = ','; 
       *curr_++ = '\"'; 
       memcpy(curr_, name, strlen(name)); 
       curr_ += strlen(name); 
       *curr_++ = '\"'; 
       *curr_++ = ':'; 
       *curr_++ = '\"'; 
    }
  }

  inline char *get_addr() {
    return curr_;
  }

  inline void add_attribute_value(int len) {
       curr_+=len; 
       *curr_++ = '\"'; 
  }

  inline void add_attribute_value(const char *value) {
       auto len = strlen(value);
       memcpy(curr_, value, len); 
       curr_+=len; 
       *curr_++ = '\"'; 
  }

  inline void start_vector(const char *name) {
       *curr_++ = ','; 
       *curr_++ = '\"'; 
       memcpy(curr_, name, strlen(name)); 
       curr_+=strlen(name); 
       *curr_++ = '\"'; 
       *curr_++ = ':'; 
       *curr_++ = '[';
       first_element = 1; 
  }

  inline void end_vector() {
       *curr_++ = ']';
  }

  inline void start_vector_element() {
       if (first_element) { 
          first_element = 0; 
       } 
       else 
          *curr_++ = ','; 
       *curr_++ = '{'; 
       first_attribute = 1; 
  }

  inline void end_vector_element() {
       *curr_++ = '}'; \
   }

  std::string_view to_string_view() { return std::string_view(buffer_, curr_ - buffer_); }
  
  char *buffer_;
  char *curr_;
  int max_len_;
  int first_element = 1; 
  int first_attribute = 1; 
};

inline json_buffer& to_json(json_buffer& out, const silkworm::Transaction& transaction) {
    START_OBJECT(out);

    if (!transaction.from) {
        (const_cast<silkworm::Transaction&>(transaction)).recover_sender();
    }
    if (transaction.from) {
        ADD_ATTRIBUTE_ZEROCOPY("from", address_to_hex2(GET_CURR_BUFF_ADDR(), transaction.from.value().bytes));
    }

    ADD_ATTRIBUTE_ZEROCOPY("gas", to_quantity(GET_CURR_BUFF_ADDR(), transaction.gas_limit));
    ADD_ATTRIBUTE_ZEROCOPY("input", to_hex(GET_CURR_BUFF_ADDR(), transaction.data));
    ADD_ATTRIBUTE_ZEROCOPY("nonce", to_quantity(GET_CURR_BUFF_ADDR(), transaction.nonce));

    if (transaction.to) {
        ADD_ATTRIBUTE_ZEROCOPY("to", address_to_hex2(GET_CURR_BUFF_ADDR(), transaction.to.value().bytes));
    } else {
        ADD_ATTRIBUTE("to", "0x0");
    }
    ADD_ATTRIBUTE_ZEROCOPY("type", to_quantity(GET_CURR_BUFF_ADDR(), (uint8_t)transaction.type));

    if (transaction.type != silkworm::Transaction::Type::kLegacy) {
       ADD_ATTRIBUTE_ZEROCOPY("chainId", to_quantity(GET_CURR_BUFF_ADDR(), *transaction.chain_id));
       ADD_ATTRIBUTE_ZEROCOPY("v", to_quantity(GET_CURR_BUFF_ADDR(), transaction.odd_y_parity));
       //json["accessList"] = transaction.access_list; // EIP2930
    } else {
       ADD_ATTRIBUTE_ZEROCOPY("v", to_quantity(GET_CURR_BUFF_ADDR(), silkworm::endian::to_big_compact(transaction.v())));
    }

    ADD_ATTRIBUTE_ZEROCOPY("value", to_quantity(GET_CURR_BUFF_ADDR(), transaction.value));
    ADD_ATTRIBUTE_ZEROCOPY("r", to_quantity(GET_CURR_BUFF_ADDR(), silkworm::endian::to_big_compact(transaction.r)));
    ADD_ATTRIBUTE_ZEROCOPY("s", to_quantity(GET_CURR_BUFF_ADDR(), silkworm::endian::to_big_compact(transaction.s)));
    END_OBJECT();

    return out;
}

inline json_buffer& to_json2(json_buffer& out, const silkworm::Transaction& transaction) {

    if (!transaction.from) {
        (const_cast<silkworm::Transaction&>(transaction)).recover_sender();
    }
    if (transaction.from) {
        out.add_attribute_name("from"); 
        out.add_attribute_value(address_to_hex2(out.get_addr(), transaction.from.value().bytes));
    }

    out.add_attribute_name("gas");
    out.add_attribute_value(to_quantity(out.get_addr(), transaction.gas_limit));

    out.add_attribute_name("input");
    out.add_attribute_value(to_hex(out.get_addr(), transaction.data));

    out.add_attribute_name("nonce");
    out.add_attribute_value(to_quantity(out.get_addr(), transaction.nonce));

    if (transaction.to) {
        out.add_attribute_name("to");
        out.add_attribute_value(address_to_hex2(out.get_addr(), transaction.to.value().bytes));
    } else {
        out.add_attribute_name("to");
        out.add_attribute_value("0x0");
    }
    out.add_attribute_name("type");
    out.add_attribute_value(to_quantity(out.get_addr(), (uint8_t)transaction.type));

    if (transaction.type != silkworm::Transaction::Type::kLegacy) {
       out.add_attribute_name("chainId");
       out.add_attribute_value(to_quantity(out.get_addr(), *transaction.chain_id));

       out.add_attribute_name("v");
       out.add_attribute_value(to_quantity(out.get_addr(), transaction.odd_y_parity));
       //json["accessList"] = transaction.access_list; // EIP2930
    } else {
       out.add_attribute_name("v");
       out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(transaction.v())));
    }

    out.add_attribute_name("value");
    out.add_attribute_value(to_quantity(out.get_addr(), transaction.value));

    out.add_attribute_name("r");
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(transaction.r)));

    out.add_attribute_name("s");
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(transaction.s)));

    return out;
}

inline json_buffer& to_json(json_buffer& out, const silkworm::BlockHeader& header) {
    START_OBJECT(out);
    ADD_ATTRIBUTE_ZEROCOPY("number", to_quantity(GET_CURR_BUFF_ADDR(), header.number));
    ADD_ATTRIBUTE_ZEROCOPY("parentHash", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), header.parent_hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("nonce", nonce_to_hex2(GET_CURR_BUFF_ADDR(), header.nonce.data()));
    ADD_ATTRIBUTE_ZEROCOPY("sha3Uncles", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), header.ommers_hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("logsBloom", bloom_to_hex2(GET_CURR_BUFF_ADDR(), header.logs_bloom.data()));
    ADD_ATTRIBUTE_ZEROCOPY("transactionsRoot", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), header.transactions_root.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("stateRoot", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), header.state_root.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("receiptsRoot", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), header.receipts_root.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("miner", address_to_hex2(GET_CURR_BUFF_ADDR(), header.beneficiary.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("extraData", to_hex(GET_CURR_BUFF_ADDR(), header.extra_data));
    ADD_ATTRIBUTE_ZEROCOPY("difficulty", to_quantity(GET_CURR_BUFF_ADDR(), silkworm::endian::to_big_compact(header.difficulty)));
    ADD_ATTRIBUTE_ZEROCOPY("mixHash", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), header.mix_hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("gasLimit", to_quantity(GET_CURR_BUFF_ADDR(), header.gas_limit));
    ADD_ATTRIBUTE_ZEROCOPY("gasUsed", to_quantity(GET_CURR_BUFF_ADDR(), header.gas_used));
    ADD_ATTRIBUTE_ZEROCOPY("timestamp", to_quantity(GET_CURR_BUFF_ADDR(), header.timestamp));

    if (header.base_fee_per_gas.has_value()) {
       ADD_ATTRIBUTE_ZEROCOPY("baseFeePerGas", to_quantity(GET_CURR_BUFF_ADDR(), header.base_fee_per_gas.value_or(0)));
    }

    END_OBJECT();
    return out;
}

inline json_buffer& to_json2(json_buffer& out, const silkworm::BlockHeader& header) {
    out.add_attribute_name("number");
    out.add_attribute_value(to_quantity(out.get_addr(), header.number));

    out.add_attribute_name("parentHash");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.parent_hash.bytes));

    out.add_attribute_name("nonce");
    out.add_attribute_value(nonce_to_hex2(out.get_addr(), header.nonce.data()));

    out.add_attribute_name("sha3Uncles");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.ommers_hash.bytes));

    out.add_attribute_name("logsBloom");
    out.add_attribute_value(bloom_to_hex2(out.get_addr(), header.logs_bloom.data()));

    out.add_attribute_name("transactionsRoot");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.transactions_root.bytes));

    out.add_attribute_name("stateRoot");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.state_root.bytes));

    out.add_attribute_name("receiptsRoot");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.receipts_root.bytes));

    out.add_attribute_name("miner");
    out.add_attribute_value(address_to_hex2(out.get_addr(), header.beneficiary.bytes));

    out.add_attribute_name("extraData");
    out.add_attribute_value(to_hex(out.get_addr(), header.extra_data));

    out.add_attribute_name("difficulty");
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(header.difficulty)));

    out.add_attribute_name("mixHash");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), header.mix_hash.bytes));

    out.add_attribute_name("gasLimit");
    out.add_attribute_value(to_quantity(out.get_addr(), header.gas_limit));

    out.add_attribute_name("gasUsed");
    out.add_attribute_value(to_quantity(out.get_addr(), header.gas_used));

    out.add_attribute_name("timestamp");
    out.add_attribute_value(to_quantity(out.get_addr(), header.timestamp));

    if (header.base_fee_per_gas.has_value()) {
       out.add_attribute_name("baseFeePerGas");
       out.add_attribute_value(to_quantity(out.get_addr(), header.base_fee_per_gas.value_or(0)));
    }

    return out;
}

inline size_t copy_bn (char *dest, char *src, int len) {
   memcpy(dest, src, len);
   return len;
}

inline json_buffer& to_json(json_buffer& out, const silkrpc::Block& b) {
    char buffer[100];
    const auto block_number_size = to_quantity(buffer, b.block.header.number); 
    START_OBJECT(out);
    ADD_ATTRIBUTE_ZEROCOPY("number", copy_bn(GET_CURR_BUFF_ADDR(), buffer, block_number_size));
    ADD_ATTRIBUTE_ZEROCOPY("hash", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("parentHash", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.parent_hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("nonce", nonce_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.nonce.data()));
    ADD_ATTRIBUTE_ZEROCOPY("sha3Uncles", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.ommers_hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("logsBloom", bloom_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.logs_bloom.data()));
    ADD_ATTRIBUTE_ZEROCOPY("transactionsRoot", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.transactions_root.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("stateRoot", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.state_root.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("receiptsRoot", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.receipts_root.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("miner", address_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.beneficiary.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("difficulty", to_quantity(GET_CURR_BUFF_ADDR(), silkworm::endian::to_big_compact(b.block.header.difficulty)));
    ADD_ATTRIBUTE_ZEROCOPY("totalDifficulty", to_quantity(GET_CURR_BUFF_ADDR(), silkworm::endian::to_big_compact(b.total_difficulty)));
    ADD_ATTRIBUTE_ZEROCOPY("extraData", to_hex(GET_CURR_BUFF_ADDR(), b.block.header.extra_data));
    ADD_ATTRIBUTE_ZEROCOPY("mixHash", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.block.header.mix_hash.bytes));
    ADD_ATTRIBUTE_ZEROCOPY("size", to_quantity(GET_CURR_BUFF_ADDR(), b.get_block_size()));
    ADD_ATTRIBUTE_ZEROCOPY("gasLimit", to_quantity(GET_CURR_BUFF_ADDR(), b.block.header.gas_limit));
    ADD_ATTRIBUTE_ZEROCOPY("gasUsed", to_quantity(GET_CURR_BUFF_ADDR(), b.block.header.gas_used));
    if (b.block.header.base_fee_per_gas.has_value()) {
       ADD_ATTRIBUTE_ZEROCOPY("baseFeePerGas", to_quantity(GET_CURR_BUFF_ADDR(), b.block.header.base_fee_per_gas.value_or(0))); }
    ADD_ATTRIBUTE_ZEROCOPY("timestamp", to_quantity(GET_CURR_BUFF_ADDR(), b.block.header.timestamp));

    if (b.full_tx) {
        int n_elem =  b.block.transactions.size(); 
        START_VECTOR("transactions");
        for (auto i{0}; i < n_elem; i++) {
            START_ELEMENT(out);
            to_json(out, b.block.transactions[i]);
            RESTORE();  //XXX
            ADD_ATTRIBUTE_ZEROCOPY("transactionIndex", to_quantity(GET_CURR_BUFF_ADDR(), i));
            ADD_ATTRIBUTE_ZEROCOPY("blockhash", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), b.hash.bytes));
            ADD_ATTRIBUTE_ZEROCOPY("blockNumber", copy_bn(GET_CURR_BUFF_ADDR(), buffer, block_number_size));
            //ADD_ATTRIBUTE_ZEROCOPY("blockNumber", to_quantity(GET_CURR_BUFF_ADDR(), b.block.header.number)); // XXX
            ADD_ATTRIBUTE_ZEROCOPY("gasPrice", to_quantity(GET_CURR_BUFF_ADDR(), b.block.transactions[i].effective_gas_price(b.block.header.base_fee_per_gas.value_or(0))));
            END_ELEMENT();
        }
        END_VECTOR();
    }

    std::vector<evmc::bytes32> ommer_hashes;
    ommer_hashes.reserve(b.block.ommers.size());
    for (auto i{0}; i < b.block.ommers.size(); i++) {
        //ommer_hashes.emplace(ommer_hashes.end(), std::move(b.block.ommers[i].hash()));
    }
    //ADD_ATTRIBUTE_ZEROCOPY("uncles", bytes32_to_hex2(GET_CURR_BUFF_ADDR(), ommer_hashes)); XXX

    END_OBJECT();
    return out;
}

inline json_buffer& to_json2(json_buffer& out, const silkrpc::Block& b) {
    char buffer[100];
    const auto block_number_size = to_quantity(buffer, b.block.header.number); 
    out.add_attribute_name("number");
    out.add_attribute_value(copy_bn(out.get_addr(), buffer, block_number_size));

    out.add_attribute_name("hash");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.hash.bytes));

    out.add_attribute_name("parentHash");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.parent_hash.bytes));

    out.add_attribute_name("nonce");
    out.add_attribute_value(nonce_to_hex2(out.get_addr(), b.block.header.nonce.data()));

    out.add_attribute_name("sha3Uncles");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.ommers_hash.bytes));

    out.add_attribute_name("logsBloom");
    out.add_attribute_value(bloom_to_hex2(out.get_addr(), b.block.header.logs_bloom.data()));

    out.add_attribute_name("transactionsRoot");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.transactions_root.bytes));

    out.add_attribute_name("stateRoot");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.state_root.bytes));

    out.add_attribute_name("receiptsRoot");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.receipts_root.bytes));

    out.add_attribute_name("miner");
    out.add_attribute_value(address_to_hex2(out.get_addr(), b.block.header.beneficiary.bytes));

    out.add_attribute_name("difficulty");
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(b.block.header.difficulty)));

    out.add_attribute_name("totalDifficulty");
    out.add_attribute_value(to_quantity(out.get_addr(), silkworm::endian::to_big_compact(b.total_difficulty)));

    out.add_attribute_name("extraData");
    out.add_attribute_value(to_hex(out.get_addr(), b.block.header.extra_data));

    out.add_attribute_name("mixHash");
    out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.block.header.mix_hash.bytes));

    out.add_attribute_name("size");
    out.add_attribute_value(to_quantity(out.get_addr(), b.get_block_size()));

    out.add_attribute_name("gasLimit");
    out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.gas_limit));

    out.add_attribute_name("gasUsed");
    out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.gas_used));

    if (b.block.header.base_fee_per_gas.has_value()) {
       out.add_attribute_name("baseFeePerGas");
       out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.base_fee_per_gas.value_or(0)));
    }
    out.add_attribute_name("timestamp");
    out.add_attribute_value(to_quantity(out.get_addr(), b.block.header.timestamp));

    if (b.full_tx) {
        int n_elem =  b.block.transactions.size(); 
        out.start_vector("transactions");
        for (auto i{0}; i < n_elem; i++) {
            out.start_vector_element();
            to_json2(out, b.block.transactions[i]);

            out.add_attribute_name("transactionIndex");
            out.add_attribute_value(to_quantity(out.get_addr(), i));

            out.add_attribute_name("blockhash");
            out.add_attribute_value(bytes32_to_hex2(out.get_addr(), b.hash.bytes));

            out.add_attribute_name("blockNumber");
            out.add_attribute_value(copy_bn(out.get_addr(), buffer, block_number_size));

            out.add_attribute_name("gasPrice");
            out.add_attribute_value(to_quantity(out.get_addr(), b.block.transactions[i].effective_gas_price(b.block.header.base_fee_per_gas.value_or(0))));
            out.end_vector_element();
        }
        out.end_vector();
    }

    std::vector<evmc::bytes32> ommer_hashes;
    ommer_hashes.reserve(b.block.ommers.size());
    out.start_vector("uncles");
    for (auto i{0}; i < b.block.ommers.size(); i++) {
#ifdef notdef
        out.start_vector_element();
        out.add_attribute_name("uncles");
        out.add_attribute_value(bytes32_to_hex2(out.get_addr(), std::move(b.block.ommers[i].hash())));
        out.end_vector_element();
#endif
    }
    out.end_vector();

    return out;
}



json_buffer encode_transaction(const Transaction& transaction) {
    char buffer[4096];
    json_buffer output_buffer{buffer, 4096};
    to_json(output_buffer, transaction);
    output_buffer.end();
    //std::cout << "Transaction: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

json_buffer encode_transaction2(const Transaction& transaction) {
    char buffer[4096];
    json_buffer output_buffer{buffer, 4096};
    to_json2(output_buffer, transaction);
    output_buffer.end();
    //std::cout << "Transaction: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

json_buffer encode_block_header(const BlockHeader& header) {
    char buffer[4096];
    json_buffer output_buffer{buffer, 4096};
    to_json(output_buffer, header);
    output_buffer.end();
    //std::cout << "BlockHeader: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

json_buffer encode_block_header2(const BlockHeader& header) {
    char buffer[4096];
    json_buffer output_buffer{buffer, 4096};
    to_json2(output_buffer, header);
    output_buffer.end();
    //std::cout << "BlockHeader: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

json_buffer encode_block(const silkrpc::Block& b) {
    //char *buffer = (char *)malloc(1*4096); // XXX
    char buffer[14096];
    json_buffer output_buffer{buffer, 4096};
    to_json(output_buffer, b);
    output_buffer.end();
    //std::cout << "Block: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

json_buffer encode_block2(const silkrpc::Block& b) {
    //char *buffer = (char *)malloc(1*4096); // XXX
    char buffer[14096];
    json_buffer output_buffer{buffer, 4096};
    to_json2(output_buffer, b);
    output_buffer.end();
    //std::cout << "Block: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}



static void benchmark_encode_transaction_macro_json(benchmark::State& state) {
    const auto transaction_json = encode_transaction(TRANSACTION_LEGACY).to_string_view();
    //std::cout << "transaction: "  << transaction_json << "\n";

      //  R"("hash":"0xa4ee16008c6596d86a7c599a74b1cda264d609558ccc2d20a722a2cd58bad6eb",)"

    ensure(transaction_json == 
        R"({"from":"0x6df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("gas":"0x12",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("to":"0x5df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("type":"0x0",)"
        R"("v":"0x1c",)"
        R"("value":"0x7a69",)"
        R"("r":"0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0",)"
        R"("s":"0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a"})");

    for (auto _ : state) {
        json_buffer output_buffer = encode_transaction(TRANSACTION_LEGACY);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_transaction_macro_json);


static void benchmark_encode_block_header_macro_json(benchmark::State& state) {
    const auto block_header_json = encode_block_header(HEADER).to_string_view();
    //std::cout << "block_header: "  << block_header_json << "\n";

    ensure(block_header_json == 
        R"({"number":"0x5",)"
        R"("parentHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("nonce":"0x0102030405060708",)"
        R"("sha3Uncles":"0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d",)"
        R"("logsBloom":"0x000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000",)"
        R"("transactionsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e",)"
        R"("stateRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d",)"
        R"("receiptsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",)"
        R"("miner":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("extraData":"0x0001ff0100",)"
        R"("difficulty":"0x",)"
        R"("mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",)"
        R"("gasLimit":"0xf4240",)"
        R"("gasUsed":"0xf4240",)"
        R"("timestamp":"0x52795d",)"
        R"("baseFeePerGas":"0x3e8"})");

    for (auto _ : state) {
        json_buffer output_buffer = encode_block_header(HEADER);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_header_macro_json);

static void benchmark_encode_block_macro_json(benchmark::State& state) {
    const auto block_json = encode_block(BLOCK).to_string_view();
    //std::cout << "block: "  << block_json << "\n";

    ensure(block_json == 
        R"({"number":"0x5",)"
        R"("hash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("parentHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("nonce":"0x0102030405060708",)"
        R"("sha3Uncles":"0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d",)"
        R"("logsBloom":"0x000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000",)"
        R"("transactionsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e",)"
        R"("stateRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d",)"
        R"("receiptsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",)"
        R"("miner":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("difficulty":"0x",)"
        R"("totalDifficulty":"0x4",)"
        R"("extraData":"0x0001ff0100",)"
        R"("mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",)"
        R"("size":"0x498",)"
        R"("gasLimit":"0xf4240",)"
        R"("gasUsed":"0xf4240",)"
        R"("baseFeePerGas":"0x3e8",)"
        R"("timestamp":"0x52795d",)"
        R"("transactions":[)"
        R"({"from":"0x6df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("gas":"0x12",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("to":"0x5df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("type":"0x0",)"
        R"("v":"0x1c",)"
        R"("value":"0x7a69",)"
        R"("r":"0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0",)"
        R"("s":"0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a",)"
        R"("transactionIndex":"0x0",)"
        R"("blockhash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("blockNumber":"0x5",)"
        R"("gasPrice":"0x2d79883d2000"},)"
        R"({"from":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053",)"
        R"("gas":"0x0",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("type":"0x1",)"
        R"("chainId":"0x0",)"
        R"("v":"0x0",)"
        R"("value":"0x0",)"
        R"("r":"0x1",)"
        R"("s":"0x12",)"
        R"("transactionIndex":"0x1",)"
        R"("blockhash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("blockNumber":"0x5",)"
        R"("gasPrice":"0x4a817c800"}]})");


    for (auto _ : state) {
        json_buffer output_buffer = encode_block(BLOCK);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_macro_json);

static void benchmark_encode_transaction_class_json(benchmark::State& state) {
    const auto transaction_json = encode_transaction2(TRANSACTION_LEGACY).to_string_view();
    //std::cout << "transaction: "  << transaction_json << "\n";

      //  R"("hash":"0xa4ee16008c6596d86a7c599a74b1cda264d609558ccc2d20a722a2cd58bad6eb",)"

    ensure(transaction_json == 
        R"({"from":"0x6df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("gas":"0x12",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("to":"0x5df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("type":"0x0",)"
        R"("v":"0x1c",)"
        R"("value":"0x7a69",)"
        R"("r":"0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0",)"
        R"("s":"0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a"})");

    for (auto _ : state) {
        json_buffer output_buffer = encode_transaction2(TRANSACTION_LEGACY);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_transaction_class_json);

static void benchmark_encode_block_header_class_json(benchmark::State& state) {
    const auto block_header_json = encode_block_header2(HEADER).to_string_view();
    //std::cout << "block_header: "  << block_header_json << "\n";

    ensure(block_header_json == 
        R"({"number":"0x5",)"
        R"("parentHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("nonce":"0x0102030405060708",)"
        R"("sha3Uncles":"0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d",)"
        R"("logsBloom":"0x000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000",)"
        R"("transactionsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e",)"
        R"("stateRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d",)"
        R"("receiptsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",)"
        R"("miner":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("extraData":"0x0001ff0100",)"
        R"("difficulty":"0x",)"
        R"("mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",)"
        R"("gasLimit":"0xf4240",)"
        R"("gasUsed":"0xf4240",)"
        R"("timestamp":"0x52795d",)"
        R"("baseFeePerGas":"0x3e8"})");

    for (auto _ : state) {
        json_buffer output_buffer = encode_block_header2(HEADER);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_header_class_json);

static void benchmark_encode_block_class_json(benchmark::State& state) {
    const auto block_json = encode_block2(BLOCK).to_string_view();
    //std::cout << "block: "  << block_json << "\n";


    ensure(block_json == 
        R"({"number":"0x5",)"
        R"("hash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("parentHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("nonce":"0x0102030405060708",)"
        R"("sha3Uncles":"0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d",)"
        R"("logsBloom":"0x000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000",)"
        R"("transactionsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e",)"
        R"("stateRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d",)"
        R"("receiptsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",)"
        R"("miner":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("difficulty":"0x",)"
        R"("totalDifficulty":"0x4",)"
        R"("extraData":"0x0001ff0100",)"
        R"("mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",)"
        R"("size":"0x498",)"
        R"("gasLimit":"0xf4240",)"
        R"("gasUsed":"0xf4240",)"
        R"("baseFeePerGas":"0x3e8",)"
        R"("timestamp":"0x52795d",)"
        R"("transactions":[)"
        R"({"from":"0x6df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("gas":"0x12",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("to":"0x5df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("type":"0x0",)"
        R"("v":"0x1c",)"
        R"("value":"0x7a69",)"
        R"("r":"0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0",)"
        R"("s":"0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a",)"
        R"("transactionIndex":"0x0",)"
        R"("blockhash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("blockNumber":"0x5",)"
        R"("gasPrice":"0x2d79883d2000"},)"
        R"({"from":"0x007fb8417eb9ad4d958b050fc3720d5b46a2c053",)"
        R"("gas":"0x0",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("to":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("type":"0x1",)"
        R"("chainId":"0x0",)"
        R"("v":"0x0",)"
        R"("value":"0x0",)"
        R"("r":"0x1",)"
        R"("s":"0x12",)"
        R"("transactionIndex":"0x1",)"
        R"("blockhash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("blockNumber":"0x5",)"
        R"("gasPrice":"0x4a817c800"}],)"
        R"("uncles":[]})");

    for (auto _ : state) {
        json_buffer output_buffer = encode_block2(BLOCK);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_class_json);


inline output_buffer& operator<<(output_buffer& out, const Block& block) {
    li::json_object(
        s::transactions = li::json_vector(
                                        s::data(li::json_key("input")), // PXB:: TRANS_INDEX 
                                        //s::hash(li::json_key("hash")),
                                        //s::get_block_number(li::json_key("number")),
                                        s::get_gas_limit(li::json_key("gas")), // PXB:: GAS_PRICE 
                                        s::from(li::json_key("from")),
                                        s::get_gas_limit(li::json_key("gas")),
                                        //s::get_hash(li::json_key("hash")),
                                        s::data(li::json_key("input")),
                                        s::get_nonce(li::json_key("nonce")),
                                        s::get_r(li::json_key("r")),
                                        s::get_s(li::json_key("s")),
                                        s::to(li::json_key("to")),
                                        s::get_type(li::json_key("type")),
                                        s::get_v(li::json_key("v")),
                                        //s::max_fee_per_gas,
                                        s::get_value(li::json_key("value"))),

        s::ommers =    li::json_vector( s::parent_hash(li::json_key("parentHash"))),

        s::header  = li::json_object(
                                        s::get_block_number(li::json_key("number")),
                                        //s::hash(li::json_key("hash")),
                                        s::parent_hash(li::json_key("parentHash")),
                                        s::nonce,
                                        s::ommers_hash(li::json_key("sha3Uncles")),
                                        s::logs_bloom(li::json_key("logsBloom")),
                                        s::transactions_root(li::json_key("transactionsRoot")),
                                        s::state_root(li::json_key("stateRoot")),
                                        s::receipts_root(li::json_key("receiptsRoot")),
                                        s::beneficiary(li::json_key("miner")),
                                        s::difficulty,
                                        //s::total_difficulty,
                                        s::extra_data(li::json_key("extraData")),
                                        s::mix_hash(li::json_key("mixHash")),
                                        //s::get_block_size(li::json_key("size")),
                                        s::get_gas_limit(li::json_key("gas_limit")),
                                        s::get_gas_used(li::json_key("gas_used")),
                                        s::get_timestamp(li::json_key("timestamp")))).encode(out, block);

       return out;
}

void *encode_class_child(void *arg) {
    int i;
    for (i = 0; i < MAX_ENCODE; i++) {
        json_buffer output_buffer = encode_block2(BLOCK); // BIG_BLOCK
        output_buffer.reset();
    }
    return 0;
}

void encode_class() {
    int i;

    for (i = 0; i < MAX_THRS; i++) {
        pthread_create(&tid[i], NULL, &encode_class_child, NULL);
    }
    for (i = 0; i < MAX_THRS; i++) {
        pthread_join(tid[i], NULL);
    }
}

static void benchmark_encode_block_class_batch_json(benchmark::State& state) {
    for (auto _ : state) {
       encode_class();
    }
}
BENCHMARK(benchmark_encode_block_class_batch_json);

} // namespace li

li::output_buffer encode_block_header_lit(const BlockHeader& block_header) {
    char buffer[2048];
    li::output_buffer output_buffer{buffer, 2048};
    output_buffer << block_header;
    //std::cout << "block_header: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

li::output_buffer encode_transaction_lit(const Transaction& transaction) {
    char buffer[2048];
    li::output_buffer output_buffer{buffer, 2048};
    output_buffer << transaction;
    //std::cout << "transaction: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

li::output_buffer encode_block_lit(const Block& block) {
    char buffer[4096];
    li::output_buffer output_buffer{buffer, 4096};
    output_buffer << block;
    //std::cout << "block: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

static void benchmark_encode_transaction_lithium_json(benchmark::State& state) {
    const auto transaction_json = encode_transaction_lit(TRANSACTION_LEGACY).to_string_view();
    //std::cout << "transaction: "  << transaction_json << "\n";

      //  R"("hash":"0xa4ee16008c6596d86a7c599a74b1cda264d609558ccc2d20a722a2cd58bad6eb",)"
    ensure(transaction_json == 
        R"({"from":"0x6df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("gas":"0x12",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("r":"0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0",)"
        R"("s":"0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a",)"
        R"("to":"0x5df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("type":"0x0",)"
        R"("v":"0x1c",)"
        R"("value":"0x7a69"})");

    for (auto _ : state) {
        li::output_buffer output_buffer = encode_transaction_lit(TRANSACTION_LEGACY);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_transaction_lithium_json);

static void benchmark_encode_block_header_lithium_json(benchmark::State& state) {
    const auto block_header_json = encode_block_header_lit(HEADER).to_string_view();
    //std::cout << "block_header: "  << block_header_json << "\n";
    ensure(block_header_json == R"({"parentHash":"0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c",)"
        R"("sha3Uncles":"0x474f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126d",)"
        R"("miner":"0x0715a7794a1dc8e42615f059dd6e406a6594651a",)"
        R"("stateRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126d",)"
        R"("transactionsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126e",)"
        R"("receiptsRoot":"0xb02a3b0ee16c858afaa34bcd6770b3c20ee56aa2f75858733eb0e927b5b7126f",)"
        R"("logsBloom":"0x000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(000000000000000000000000000000000000000000000000000000000000000000000000)"
                    R"(00000000000000000000000000000000000000000000000000000000000000000000000000000000",)"
        R"("difficulty":"0x",)"
        R"("number":"0x5",)"
        R"("gas_limit":"0xf4240",)"
        R"("gas_used":"0xf4240",)"
        R"("timestamp":"0x52795d",)"
        R"("extraData":"0x0001ff0100",)"
        R"("mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",)"
        R"("nonce":"0x0102030405060708"})");

    for (auto _ : state) {
        li::output_buffer output_buffer = encode_block_header_lit(HEADER);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_header_lithium_json);


static void benchmark_encode_block_lithium_json(benchmark::State& state) {
    const auto block = encode_block_lit(BLOCK1).to_string_view();
    //std::cout << "block: "  << block << "\n";


/*
    ensure(transaction_json == 
        R"({"from":"0x6df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("gas":"18",)"
        R"("hash":"0xa4ee16008c6596d86a7c599a74b1cda264d609558ccc2d20a722a2cd58bad6eb",)"
        R"("input":"0x",)"
        R"("nonce":"0x0",)"
        R"("r":"0x88ff6cf0fefd94db46111149ae4bfc179e9b94721fffd821d38d16464b3f71d0",)"
        R"("s":"0x45e0aff800961cfce805daef7016b9b675c137a6a41a548f7b60a3484c06a33a",)"
        R"("to":"0x5df9b87991262f6ba471f09758cde1c0fc1de734",)"
        R"("type":"0x0",)"
        R"("v":"0x1c",)"
        R"("value":"0x7a69"})");
*/


    for (auto _ : state) {
        li::output_buffer output_buffer = encode_block_lit(BLOCK1);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_lithium_json);

BENCHMARK_MAIN();

