/*
   Copyright 2022 The Silkrpc Authors

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

#ifndef SILKRPC_JSON_STREAM_HPP_
#define SILKRPC_JSON_STREAM_HPP_

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <nlohmann/json.hpp>

namespace json {
class Stream {
public:
    explicit Stream(boost::asio::ip::tcp::socket& socket) : socket_(socket) {}

    boost::asio::awaitable<void> flush();
    boost::asio::awaitable<void> write_json(const nlohmann::json& json);

private:
    boost::asio::ip::tcp::socket& socket_;
};

} // namespace json

#endif  // SILKRPC_JSON_STREAM_HPP_
