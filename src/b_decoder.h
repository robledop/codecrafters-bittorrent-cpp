#pragma once
#include <string>

#include "lib/nlohmann/json.hpp"


using json = nlohmann::json;

class BDecoder
{
public:
    static auto decode_bencoded_value(const std::string& encoded_value, size_t& pos) -> json;
    static auto encode_bencoded_value(const json& json) -> std::string;

private:
    static auto decode_integer(const std::string& encoded_value, size_t& pos) -> int64_t;
    static auto decode_string_length(const std::string& encoded_value, size_t pos, size_t& colon_index) -> int64_t;
    static auto decode_string(const std::string& encoded_value, size_t& pos) -> std::string;
    static auto decode_list(const std::string& encoded_value, size_t& pos) -> json;
    static auto decode_dictionary(const std::string& encoded_value, size_t& pos) -> json;

    static auto encode_integer(int64_t value) -> std::string;
    static auto encode_string_length(int64_t value) -> std::string;
    static auto encode_string(const std::string& value) -> std::string;
    static auto encode_list(const json& json) -> std::string;
    static auto encode_dictionary(json json) -> std::string;
};
