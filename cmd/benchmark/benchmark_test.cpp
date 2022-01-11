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
#include <string>
#include <valarray>
#include <vector>

#include <evmc/evmc.hpp>
#include <benchmark/benchmark.h>
#include <boost/algorithm/hex.hpp>
#include <intx/intx.hpp>
#include <nlohmann/json.hpp>
#include <silkworm/common/util.hpp>

#include <silkrpc/json/lithium.hpp>
#include <silkrpc/json/lithium_json.hpp>
#include <silkrpc/json/types.hpp>
#include <silkrpc/types/block.hpp>
#include <silkrpc/types/transaction.hpp>

using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;
using silkworm::kGiga;

/* benchmark_encode_address */
static void benchmark_encode_address_nlohmann_json(benchmark::State& state) {
    evmc::address address{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address};
    for (auto _ : state) {
        const nlohmann::json j = address;
        const auto s = j.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
        //std::cout << s << "\n";
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(benchmark_encode_address_nlohmann_json);

template<int N>
std::array<char, N * 2> to_hex(uint8_t bytes[N]) {
    static const char* kHexDigits{"0123456789abcdef"};
    std::array<char, N * 2> hex_bytes;
    char* dest{&hex_bytes[0]};
    //for (const uint8_t& b : bytes) {
    for (auto i{0}; i<N; i++) {
        *dest++ = kHexDigits[bytes[i] >> 4];    // Hi
        *dest++ = kHexDigits[bytes[i] & 0x0f];  // Lo
    }
    return hex_bytes;
}

static void benchmark_encode_address_lithium_json1(benchmark::State& state) {
    evmc::address address{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address};
    static constexpr int kHexAddressSize = 2 + 20 * 2;
    for (auto _ : state) {
        char b[kHexAddressSize] = {'0', 'x'};
        li::output_buffer buffer{b, kHexAddressSize};
        const auto dest{to_hex<20>(address.bytes)};
        li::json_encode(buffer, dest);
        buffer.reset();
    }
}
BENCHMARK(benchmark_encode_address_lithium_json1);

static void benchmark_encode_address_lithium_json2(benchmark::State& state) {
    evmc::address address{0x0715a7794a1dc8e42615f059dd6e406a6594651a_address};
    static constexpr int kHexAddressSize = 2 + 20 * 2;
    static constexpr const char* kHexDigits{"0123456789abcdef"};

    for (auto _ : state) {
        char buffer[kHexAddressSize];
        li::output_buffer output_buffer{buffer, kHexAddressSize};
        output_buffer << "0x";

        //boost::algorithm::hex_lower(std::cbegin(address.bytes), std::cend(address.bytes), std::begin(b));
        //std::cout << buffer.to_string_view() << "\n";

        auto first = std::begin(address.bytes);
        const auto last = std::cend(address.bytes);
        for (auto v = *first; first != last; v = *++first) {
            //output_buffer << kHexDigits[v >> 4];    // Hi
            //output_buffer << kHexDigits[v & 0x0f];  // Lo
            output_buffer << kHexDigits[v >> 4] << kHexDigits[v & 0x0f];
        }
        //std::cout << "buffer.size()=" << output_buffer.size() << "\n";
        //std::cout << "buffer=" << output_buffer.to_string_view() << "\n";
        output_buffer.reset();
    }
}
BENCHMARK(benchmark_encode_address_lithium_json2);

BENCHMARK_MAIN();
