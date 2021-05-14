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

#ifndef SILKRPC_KV_ERROR_HPP
#define SILKRPC_KV_ERROR_HPP

#include <system_error>

enum class KVError {
    // value 0 reserved for no error
    rpc_start_stream_failed = 100,
    rpc_open_cursor_write_stream_failed,
    rpc_open_cursor_read_stream_failed,
    rpc_seek_write_stream_failed,
    rpc_seek_read_stream_failed,
    rpc_next_write_stream_failed,
    rpc_next_read_stream_failed,
    rpc_close_cursor_write_stream_failed,
    rpc_close_cursor_read_stream_failed,
    rpc_end_stream_failed,
};

namespace std {

template<>
struct is_error_code_enum<KVError> : true_type {};

} // namespace std

std::error_code make_error_code(KVError errc);

#endif // SILKRPC_KV_ERROR_HPP
