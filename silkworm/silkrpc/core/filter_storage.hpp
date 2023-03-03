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

#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>

#include <silkworm/silkrpc/types/filter.hpp>

namespace silkrpc::filter {

struct FilterEntry {
    std::chrono::system_clock::time_point last_access;
    Filter filter;
};

class FilterStorage {
public:
    explicit FilterStorage(double filter_duration = DEFAULT_FILTER_DURATION) :
       filter_duration_{filter_duration} {}

    FilterStorage(const FilterStorage&) = delete;
    FilterStorage& operator=(const FilterStorage&) = delete;

    std::string add_filter(const Filter& filter);
    bool remove_filter(const std::string& filter_id);
    std::optional<Filter> get_filter(const std::string& filter_id);

    auto size() const {
        return storage_.size();
    };

private:
    static const std::size_t DEFAULT_FILTER_DURATION = 0x800;

    // std::string generate_id() {
    //     std::stringstream stream;
    //     stream << std::hex << random_engine();    
    //     return stream.str();
    // }

    std::chrono::duration<double> filter_duration_;
    std::mutex mutex_;
    std::map<std::string, FilterEntry> storage_;
    // std::default_random_engine random_engine{std::random_device{}()};
    std::mt19937_64 random_engine{std::random_device{}()};
};

} // namespace silkrpc::filter

