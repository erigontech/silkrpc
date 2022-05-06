namespace silkrpc {

    // generates jwt token
    void generate_jwt_token(const std::string& file_path, std::string& jwt_token);

    // load a jwt secret token from provided file path
    // if the file doesn't contain the jwt secret token then we generate one
    bool obtain_jwt_token(const std::string& file_path, std::string& jwt_token);
}