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

#include <iomanip>
#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <grpcpp/grpcpp.h>

#include <silkworm/common/util.hpp>
#include <silkrpc/common/constants.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/interfaces/remote/kv.grpc.pb.h>

ABSL_FLAG(std::string, table, "", "database table name");
ABSL_FLAG(std::string, key, "", "key as hex string w/o leading 0x");
ABSL_FLAG(std::string, subkey, "", "subkey as hex string w/o leading 0x");
ABSL_FLAG(std::string, target, silkrpc::kDefaultTarget, "server location as string <address>:<port>");

std::ostream& operator<<(std::ostream& out, const grpc::Status& status) {
    out << "ok=" << std::boolalpha << status.ok();
    if (!status.ok()) {
        out << " error_code=" << status.error_code()
            << " error_message=" << status.error_message()
            << " error_details=" << status.error_details();
    }
    return out;
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("SeekBoth Erigon/Silkworm Key-Value (KV) remote interface to database");
    absl::ParseCommandLine(argc, argv);

    auto table_name{absl::GetFlag(FLAGS_table)};
    if (table_name.empty()) {
        std::cerr << "Parameter table is invalid: [" << table_name << "]\n";
        std::cerr << "Use --table flag to specify the name of Turbo-Geth database table\n";
        return -1;
    }

    auto key{absl::GetFlag(FLAGS_key)};
    const auto key_bytes_optional = silkworm::from_hex(key);
    if (key.empty() || !key_bytes_optional.has_value()) {
        std::cerr << "Parameter key is invalid: [" << key << "]\n";
        std::cerr << "Use --key flag to specify the key in key-value dupsort table\n";
        return -1;
    }
    const auto key_bytes = key_bytes_optional.value();

    auto subkey{absl::GetFlag(FLAGS_subkey)};
    const auto subkey_bytes_optional = silkworm::from_hex(subkey);
    if (subkey.empty() || !subkey_bytes_optional.has_value()) {
        std::cerr << "Parameter subkey is invalid: [" << subkey << "]\n";
        std::cerr << "Use --subkey flag to specify the subkey in key-value dupsort table\n";
        return -1;
    }
    const auto subkey_bytes = subkey_bytes_optional.value();

    auto target{absl::GetFlag(FLAGS_target)};
    if (target.empty() || target.find(":") == std::string::npos) {
        std::cerr << "Parameter target is invalid: [" << target << "]\n";
        std::cerr << "Use --target flag to specify the location of Turbo-Geth running instance\n";
        return -1;
    }

    // Create KV stub using insecure channel to target
    grpc::ClientContext context;

    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    const auto stub = remote::KV::NewStub(channel);
    const auto reader_writer = stub->Tx(&context);

    // Open cursor
    auto open_message = remote::Cursor{};
    open_message.set_op(remote::Op::OPEN);
    open_message.set_bucketname(table_name);
    auto success = reader_writer->Write(open_message);
    if (!success) {
        std::cerr << "KV stream closed sending OPEN operation req\n";
        std::cout << "KV Tx STATUS: " << reader_writer->Finish() << "\n";
        return -1;
    }
    std::cout << "KV Tx OPEN -> table_name: " << table_name << "\n";
    auto open_pair = remote::Pair{};
    success = reader_writer->Read(&open_pair);
    if (!success) {
        std::cerr << "KV stream closed receiving OPEN operation rsp\n";
        std::cout << "KV Tx STATUS: " << reader_writer->Finish() << "\n";
        return -1;
    }
    auto cursor_id = open_pair.cursorid();
    std::cout << "KV Tx OPEN <- cursor: " << cursor_id << "\n";

    // Seek given key in given table
    auto seek_both_message = remote::Cursor{};
    seek_both_message.set_op(remote::Op::SEEK_BOTH);
    seek_both_message.set_cursor(cursor_id);
    seek_both_message.set_k(key_bytes.c_str(), key_bytes.length());
    seek_both_message.set_v(subkey_bytes.c_str(), subkey_bytes.length());
    success = reader_writer->Write(seek_both_message);
    if (!success) {
        std::cerr << "KV stream closed sending SEEK_BOTH operation req\n";
        std::cout << "KV Tx STATUS: " << reader_writer->Finish() << "\n";
        return -1;
    }
    std::cout << "KV Tx SEEK_BOTH -> cursor: " << cursor_id << " key: " << key_bytes << " subkey: " << subkey_bytes << "\n";
    auto seek_both_pair = remote::Pair{};
    success = reader_writer->Read(&seek_both_pair);
    if (!success) {
        std::cerr << "KV stream closed receiving SEEK_BOTH operation rsp\n";
        std::cout << "KV Tx STATUS: " << reader_writer->Finish() << "\n";
        return -1;
    }
    const auto& rsp_key_bytes = silkworm::byte_view_of_string(seek_both_pair.k());
    const auto& rsp_value_bytes = silkworm::byte_view_of_string(seek_both_pair.v());
    std::cout << "KV Tx SEEK_BOTH <- key: " << rsp_key_bytes << " value: " << rsp_value_bytes << std::endl;

    // Close cursor
    auto close_message = remote::Cursor{};
    close_message.set_op(remote::Op::CLOSE);
    close_message.set_cursor(cursor_id);
    success = reader_writer->Write(close_message);
    if (!success) {
        std::cerr << "KV stream closed sending CLOSE operation req\n";
        std::cout << "KV Tx STATUS: " << reader_writer->Finish() << "\n";
        return -1;
    }
    std::cout << "KV Tx CLOSE -> cursor: " << cursor_id << "\n";
    auto close_pair = remote::Pair{};
    success = reader_writer->Read(&close_pair);
    if (!success) {
        std::cerr << "KV stream closed receiving CLOSE operation rsp\n";
        std::cout << "KV Tx STATUS: " << reader_writer->Finish() << "\n";
        return -1;
    }
    std::cout << "KV Tx CLOSE <- cursor: " << close_pair.cursorid() << "\n";

    reader_writer->WritesDone();
    grpc::Status status = reader_writer->Finish();
    std::cout << "KV Tx STATUS: " << status << "\n";

    return 0;
}
