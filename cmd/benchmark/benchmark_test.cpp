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

#include <catch2/catch.hpp>
#include <evmc/evmc.hpp>
#include <benchmark/benchmark.h>
#include <intx/int128.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>
#include <silkworm/common/endian.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/types/bloom.hpp>

#include <silkrpc/json/lithium.hpp>
#include <silkrpc/json/lithium_json.hpp>
#include <silkrpc/json/lithium_symbol.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/transaction.hpp>

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
//BENCHMARK(benchmark_encode_address_nlohmann_json);

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
//BENCHMARK(benchmark_encode_address_lithium_json);

/* benchmark_decode_address */
static void benchmark_decode_address_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const auto address_json = nlohmann::json::parse(ADDR_STRING);
        const auto address = address_json.get<evmc::address>();
        benchmark::DoNotOptimize(address);
    }
}
//BENCHMARK(benchmark_decode_address_nlohmann_json);

static void benchmark_decode_address_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        std::string_view hex;
        li::json_decode(ADDR_STRING, hex);
        evmc::address address;
        address_from_hex(address.bytes, hex);
    }
}
//BENCHMARK(benchmark_decode_address_lithium_json);

/* benchmark_encode_bytes32 */
static void benchmark_encode_bytes32_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = HASH;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
    }
}
//BENCHMARK(benchmark_encode_bytes32_nlohmann_json);

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
BENCHMARK(benchmark_encode_bytes32_lithium_json);

/* benchmark_decode_bytes32 */
static void benchmark_decode_bytes32_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const auto bytes32_json = nlohmann::json::parse(HASH_STRING);
        const auto hash = bytes32_json.get<evmc::bytes32>();
        benchmark::DoNotOptimize(hash);
    }
}
//BENCHMARK(benchmark_decode_bytes32_nlohmann_json);

static void benchmark_decode_bytes32_lithium_json(benchmark::State& state) {
    for (auto _ : state) {
        std::string_view hex;
        li::json_decode(HASH_STRING, hex);
        evmc::bytes32 hash;
        bytes32_from_hex(hash.bytes, hex);
    }
}
//BENCHMARK(benchmark_decode_bytes32_lithium_json);

/* benchmark_decode_uint256 */
static const std::string UINT256_STRING{"\"0x752f02b1438be7f67ebf0e71310db3514b162fb169cdb95ad15dde38eff7719b\""};

static void benchmark_decode_uint256_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const auto uint256_json = nlohmann::json::parse(UINT256_STRING);
        const auto i = uint256_json.get<intx::uint256>();
        benchmark::DoNotOptimize(i);
    }
}
//BENCHMARK(benchmark_decode_uint256_nlohmann_json);

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
//BENCHMARK(benchmark_decode_uint256_lithium_json);

/* benchmark_encode_block_header */
static const silkworm::BlockHeader HEADER{
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
};

static void benchmark_encode_block_header_nlohmann_json(benchmark::State& state) {
    for (auto _ : state) {
        const nlohmann::json j = HEADER;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(benchmark_encode_block_header_nlohmann_json);

#ifndef LI_SYMBOL_parent_hash
#define LI_SYMBOL_parent_hash
    LI_SYMBOL(parent_hash)
#endif // LI_SYMBOL_parent_hash

#ifndef LI_SYMBOL_ommers_hash
#define LI_SYMBOL_ommers_hash
    LI_SYMBOL(ommers_hash)
#endif // LI_SYMBOL_ommers_hash

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

/*std::string to_hex_no_leading_zeros(uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    silkworm::endian::store_big_u64(&number_bytes[0], number);
    return to_hex_no_leading_zeros(number_bytes);
}*/

template<std::size_t N>
std::size_t to_quantity(std::array<char, N>& quantity_hex_bytes, silkworm::ByteView bytes) {
    return to_hex_no_leading_zeros<N>(quantity_hex_bytes, bytes);
}

std::size_t to_quantity(std::array<char, 8>& quantity_hex_bytes, uint64_t number) {
    silkworm::Bytes number_bytes(8, '\0');
    silkworm::endian::store_big_u64(number_bytes.data(), number);
    return to_hex_no_leading_zeros<8>(quantity_hex_bytes, number_bytes);
}

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

inline output_buffer& operator<<(output_buffer& out, const silkworm::BlockHeader& block_header) {
    li::json_object(
        s::parent_hash(li::json_key("parentHash")),
        s::ommers_hash(li::json_key("sha3Uncles")),
        s::beneficiary(li::json_key("miner")),
        s::state_root(li::json_key("stateRoot")),
        s::transactions_root(li::json_key("transactionsRoot")),
        s::receipts_root(li::json_key("receiptsRoot")),
        s::logs_bloom(li::json_key("logsBloom")),
        s::difficulty,
        //s::number,
        //s::gas_limit,
        //s::gas_used,
        //s::timestamp,
        s::extra_data(li::json_key("extraData")),
        s::mix_hash(li::json_key("mixHash")),
        s::nonce).encode(out, block_header);
    return out;
}

} // namespace li

li::output_buffer encode_block_header(const silkworm::BlockHeader& block_header) {
    char buffer[2048];
    li::output_buffer output_buffer{buffer, 2048};
    output_buffer << block_header;
    //std::cout << "block_header: "  << output_buffer.to_string_view() << "\n";
    return output_buffer;
}

static void benchmark_encode_block_header_lithium_json(benchmark::State& state) {
    const auto block_header_json = encode_block_header(HEADER).to_string_view();
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
        R"("extraData":"0x0001ff0100",)"
        R"("mixHash":"0x0000000000000000000000000000000000000000000000000000000000000001",)"
        R"("nonce":"0x0102030405060708"})");
    for (auto _ : state) {
        li::output_buffer output_buffer = encode_block_header(HEADER);
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_block_header_lithium_json);

BENCHMARK_MAIN();
