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

#include "filter_storage.hpp"

// #include <sstream>

#include <silkworm/silkrpc/common/log.hpp>
#include <silkworm/silkrpc/json/types.hpp>

namespace silkrpc::filter {

std::string FilterStorage::add_filter(const Filter& filter) {
    std::lock_guard<std::mutex> lock (mutex_);

    FilterEntry entry{std::chrono::system_clock::now(), filter};
    const auto id = to_quantity(random_engine());
    storage_.emplace(id, entry);

    return id;
}

bool FilterStorage::remove_filter(const std::string& filter_id) {
    std::lock_guard<std::mutex> lock (mutex_);

    const auto itr = storage_.find(filter_id);
    if (itr == storage_.end()) {
        return false;
    }
    storage_.erase(itr);

    return true;
}

std::optional<Filter> FilterStorage::get_filter(const std::string& filter_id) {
    std::lock_guard<std::mutex> lock (mutex_);

    const auto itr = storage_.find(filter_id);
    if (itr == storage_.end()) {
        return std::nullopt;
    }

    const auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = now - itr->second.last_access;
    if (diff > filter_duration_) {
        SILKRPC_INFO << "Filter  " << filter_id << " exhausted: removed" << std::endl << std::flush;
        storage_.erase(itr);
        return std::nullopt;
    }

    itr->second.last_access = now;
    return itr->second.filter;
}

// std::string FilterStorage::generate_id() {
//     std::stringstream stream;
//     stream << std::hex << random_engine();
    
//     return stream.str();
// }
} // namespace silkrpc::filter
