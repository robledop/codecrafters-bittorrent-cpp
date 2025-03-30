#pragma once
#include <string>

#include "lib/nlohmann/json.hpp"


using json = nlohmann::json;

class BDecoder
{
public:
    static auto decode_bencoded_value(const std::string& encoded_value, size_t& pos) -> json;
private:
    static auto decode_integer(const std::string& encoded_value, size_t& pos) -> int64_t;
    static auto decode_string_length(const std::string& encoded_value, size_t pos, size_t& colon_index) -> int64_t;
    static auto decode_string(const std::string& encoded_value, size_t& pos) -> std::string;
    static auto decode_list(const std::string& encoded_value, size_t& pos) -> json;
    static auto decode_dictionary(const std::string& encoded_value, size_t& pos) -> json;
};
