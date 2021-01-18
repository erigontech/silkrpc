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

#include <chrono>
#include <iomanip>
#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <asio/io_service.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/ethdb/kv/remote/kv.grpc.pb.h>

int kv_seek_async(std::string table_name, std::string target, std::string seek_key, uint32_t timeout) {
    using namespace std::chrono;
    using namespace silkworm;

    // Create KV stub using insecure channel to target
    grpc::ClientContext context;
    grpc::CompletionQueue queue;
    grpc::Status status;
    void * got_tag;
    bool ok;

    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    const auto stub = remote::KV::NewStub(channel);

    // Prepare RPC call context and stream
    context.set_initial_metadata_corked(true); // Initial metadata coalasced with first write message
    context.set_deadline(std::chrono::system_clock::system_clock::now() + std::chrono::milliseconds{timeout});
    const auto reader_writer = stub->PrepareAsyncTx(&context, &queue);

    const uint START_TAG = 0;
    const uint OPEN_TAG = 1;
    const uint SEEK_TAG = 2;
    const uint CLOSE_TAG = 3;
    const uint FINISH_TAG = 4;

    // 1) StartCall (+ Next if initial metadata not coalesced)
    reader_writer->StartCall((void *)START_TAG);

    // 2) Open cursor
    std::cout << "KV Tx OPEN -> table_name: " << table_name << "\n";
    // 2.1) Write + Next
    auto open_message = remote::Cursor{};
    open_message.set_op(remote::Op::OPEN);
    open_message.set_bucketname(table_name);
    reader_writer->Write(open_message, (void *)OPEN_TAG);
    bool has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)OPEN_TAG) {
        return -1;
    }
    // 2.2) Read + Next
    auto open_pair = remote::Pair{};
    reader_writer->Read(&open_pair, (void *)OPEN_TAG);
    has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)OPEN_TAG) {
        return -1;
    }
    auto cursor_id = open_pair.cursorid();
    std::cout << "KV Tx OPEN <- cursor: " << cursor_id << "\n";

    // 3) Seek given key in given table
    const auto& seek_key_bytes = from_hex(seek_key);
    std::cout << "KV Tx SEEK -> cursor: " << cursor_id << " seek_key: " << seek_key_bytes << "\n";
    // 3.1) Write + Next
    auto seek_message = remote::Cursor{};
    seek_message.set_op(remote::Op::SEEK);
    seek_message.set_cursor(cursor_id);
    seek_message.set_k(seek_key_bytes.c_str(), seek_key_bytes.length());
    reader_writer->Write(seek_message, (void *)SEEK_TAG);
    has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)SEEK_TAG) {
        return -1;
    }
    // 3.2) Read + Next
    auto seek_pair = remote::Pair{};
    reader_writer->Read(&seek_pair, (void *)SEEK_TAG);
    has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)SEEK_TAG) {
        return -1;
    }
    const auto& key_bytes = byte_view_of_string(seek_pair.k());
    const auto& value_bytes = byte_view_of_string(seek_pair.v());
    std::cout << "KV Tx SEEK <- key: " << key_bytes << " value: " << value_bytes << std::endl;

    // 4) Close cursor
    std::cout << "KV Tx CLOSE -> cursor: " << cursor_id << "\n";
    // 4.1) Write + Next
    auto close_message = remote::Cursor{};
    close_message.set_op(remote::Op::CLOSE);
    close_message.set_cursor(cursor_id);
    reader_writer->WriteLast(close_message, grpc::WriteOptions(), (void *)CLOSE_TAG);
    has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)CLOSE_TAG) {
        return -1;
    }
    // 4.2) Read + Next
    auto close_pair = remote::Pair{};
    reader_writer->Read(&close_pair, (void *)CLOSE_TAG);
    has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)CLOSE_TAG) {
        return -1;
    }
    std::cout << "KV Tx CLOSE <- cursor: " << close_pair.cursorid() << "\n";

    // 5) Finish
    reader_writer->Finish(&status, (void *)FINISH_TAG);
    if (!status.ok()) {
        std::cout << "KV Tx Status <- error_code: " << status.error_code() << "\n";
        std::cout << "KV Tx Status <- error_message: " << status.error_message() << "\n";
        std::cout << "KV Tx Status <- error_details: " << status.error_details() << "\n";
        return -1;
    }
    has_event = queue.Next(&got_tag, &ok);
    if (!has_event || got_tag != (void *)FINISH_TAG) {
        return -1;
    }

    return 0;
}

ABSL_FLAG(std::string, table, "", "database table name");
ABSL_FLAG(std::string, seekkey, "", "seek key as hex string w/o leading 0x");
ABSL_FLAG(std::string, target, silkrpc::common::kDefaultTarget, "server location as string <address>:<port>");
ABSL_FLAG(uint32_t, timeout, silkrpc::common::kDefaultTimeout.count(), "gRPC call timeout as 32-bit integer");

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Seek Turbo-Geth/Silkworm Key-Value (KV) remote interface to database");
    absl::ParseCommandLine(argc, argv);

    using namespace std::chrono;
    using namespace silkworm;

    auto table_name{absl::GetFlag(FLAGS_table)};
    if (table_name.empty()) {
        std::cerr << "Parameter table is invalid: [" << table_name << "]\n";
        std::cerr << "Use --table flag to specify the name of Turbo-Geth database table\n";
        return -1;
    }

    auto seek_key{absl::GetFlag(FLAGS_seekkey)};
    if (seek_key.empty() || false /*is not hex*/) {
        std::cerr << "Parameter seek key is invalid: [" << seek_key << "]\n";
        std::cerr << "Use --seekkey flag to specify the seek key in Turbo-Geth database table\n";
        return -1;
    }

    auto target{absl::GetFlag(FLAGS_target)};
    if (target.empty() || target.find(":") == std::string::npos) {
        std::cerr << "Parameter target is invalid: [" << target << "]\n";
        std::cerr << "Use --target flag to specify the location of Turbo-Geth running instance\n";
        return -1;
    }

    auto timeout{absl::GetFlag(FLAGS_timeout)};
    if (timeout < 0) {
        std::cerr << "Parameter timeout is invalid: [" << timeout << "]\n";
        std::cerr << "Use --timeout flag to specify the timeout in msecs for Turbo-Geth KV gRPC calls\n";
        return -1;
    }

    asio::io_service service;

    int rv;
    service.post([&] () {
        rv = kv_seek_async(table_name, target, seek_key, timeout);
    });

    service.run();

    return rv;
}
