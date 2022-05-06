#include <fstream>
#include <string>
#include <filesystem>

#include <evmc/evmc.hpp>
#include <silkrpc/common/log.hpp>
#include <silkrpc/types/jwt_secret.hpp>
#include <silkworm/common/util.hpp>

// generates jwt token
void silkrpc::generate_jwt_token(const std::string& file_path, std::string& jwt_token) {
    const char hex_characters[]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    // if file doesn't exist we generate one
    if (!std::filesystem::exists(file_path)) {
        SILKRPC_LOG << "Jwt file not found\n";
        SILKRPC_LOG << "Creating Jwt file: " << file_path << "\n";
        std::ofstream{file_path};
    }
    // if no token has been found then we make one
    std::ofstream write_file;
    write_file.open(file_path);

    SILKRPC_CRIT << "No jwt token found\n";
    SILKRPC_CRIT << "Generating jwt token\n";

    srand(time(0));
    // Generates a random 32 bytes hex token ( not including prefix )
    for(int i = 0; i < 64; ++i) {
        jwt_token += hex_characters[rand()%16];
    }
    SILKRPC_LOG << "Jwt token created: " << "0x" << jwt_token << "\n";
    write_file << "0x" << jwt_token << "\n";
    write_file.close();
}

// load a jwt secret token from provided file path
// if the file doesn't contain the jwt secret token then we generate one
bool silkrpc::obtain_jwt_token(const std::string& file_path, std::string& jwt_token) {
    std::ifstream read_file;
    read_file.open(file_path);

    std::getline(read_file, jwt_token);
    read_file.close();
    // get rid of prefix if we have a token
    if (jwt_token.length() > 1 && jwt_token[0] == '0' && (jwt_token[1] == 'x' || jwt_token[1] == 'X')) {
        jwt_token = jwt_token.substr(2);
    }

    if (jwt_token.length() == 64) {
        SILKRPC_LOG << "Found token: " <<  "0x" << jwt_token << "\n";
        return true;

    }
    // if token is of an incorrect size then we return an empty string
    if (jwt_token.length() != 0 && jwt_token.length() != 64) {
        return false;
    }
    // make a jwt token since we dont have one
    generate_jwt_token(file_path, jwt_token);
    return true;
}

