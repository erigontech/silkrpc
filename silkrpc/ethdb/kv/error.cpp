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

#include "error.hpp"

#include <string>

namespace {

struct KVErrorCategory : std::error_category {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char* KVErrorCategory::name() const noexcept { return "kv"; }

std::string KVErrorCategory::message(int ev) const {
    switch (static_cast<KVError>(ev)) {
        case KVError::rpc_start_stream_failed:
            return "start stream failed";
        case KVError::rpc_open_cursor_write_stream_failed:
            return "write stream failed in cursor OPEN";
        case KVError::rpc_open_cursor_read_stream_failed:
            return "read stream failed in cursor OPEN";
        case KVError::rpc_seek_write_stream_failed:
            return "write stream failed in cursor SEEK";
        case KVError::rpc_seek_read_stream_failed:
            return "read stream failed in cursor SEEK";
        case KVError::rpc_next_write_stream_failed:
            return "write stream failed in cursor NEXT";
        case KVError::rpc_next_read_stream_failed:
            return "read stream failed in cursor NEXT";
        case KVError::rpc_close_cursor_write_stream_failed:
            return "write stream failed in cursor CLOSE";
        case KVError::rpc_close_cursor_read_stream_failed:
            return "read stream failed in cursor CLOSE";
        case KVError::rpc_end_stream_failed:
            return "end stream failed";
    }
}

const KVErrorCategory kv_error_category{};

} // namespace

std::error_code make_error_code(KVError errc) {
    return {static_cast<int>(errc), kv_error_category};
}
