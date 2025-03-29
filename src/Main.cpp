#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <fstream>

#include "torrent.h"
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

auto decode_bencoded_value(const std::string& encoded_value, size_t& pos) -> json;

auto decode_integer(const std::string& encoded_value, size_t& pos) -> int64_t
{
    pos++;
    const auto end = encoded_value.find_first_of('e', pos);
    const std::string number_string = encoded_value.substr(pos, end);
    pos = end + 1;
    return std::strtoll(number_string.c_str(), nullptr, 10);
}

auto decode_string_length(const std::string& encoded_value, const size_t pos, size_t& colon_index) -> int64_t
{
    colon_index = encoded_value.find(':', pos);
    const std::string number_string = encoded_value.substr(pos, colon_index);
    const int64_t length = std::strtoll(number_string.c_str(), nullptr, 10);

    return length;
}

auto decode_string(const std::string& encoded_value, size_t& pos) -> std::string
{
    size_t colon_index = 0;
    const int64_t length = decode_string_length(encoded_value, pos, colon_index);
    std::string str = encoded_value.substr(colon_index + 1, length);
    pos = colon_index + length + 1;
    return str;
}

auto decode_list(const std::string& encoded_value, size_t& pos) -> json
{
    json list = json::array();
    pos++;

    while (encoded_value[pos] != 'e')
    {
        list.push_back(decode_bencoded_value(encoded_value, pos));
    }

    pos++;

    return list;
}

auto decode_dictionary(const std::string& encoded_value, size_t& pos) -> json
{
    json dictionary = json::object();
    pos++;

    while (encoded_value[pos] != 'e')
    {
        const auto key = decode_bencoded_value(encoded_value, pos);
        const auto value = decode_bencoded_value(encoded_value, pos);
        dictionary[key] = value;
    }

    pos++;

    return dictionary;
}

auto decode_bencoded_value(const std::string& encoded_value, size_t& pos) -> json
{
    // string
    // Example: "5:hello" -> "hello"
    if (std::isdigit(encoded_value[pos]))
    {
        return json(decode_string(encoded_value, pos));
    }

    // Integer
    // Example: "i42e" -> 42
    if (encoded_value[pos] == 'i')
    {
        return json(decode_integer(encoded_value, pos));
    }

    // List
    // Example: "l5:helloi42ee" -> ["hello", 42] 
    if (encoded_value[pos] == 'l')
    {
        return decode_list(encoded_value, pos);
    }

    // Dictionary
    // Example: d3:foo3:bar5:helloi52ee -> {"foo":"bar","hello":52}
    if (encoded_value[pos] == 'd')
    {
        return decode_dictionary(encoded_value, pos);
    }

    throw std::runtime_error("Unhandled encoded value: " + encoded_value);
}

auto main(int argc, char* argv[]) -> int
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    if (const std::string command = argv[1]; command == "decode")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        const std::string encoded_value = argv[2];
        size_t pos = 0;
        const json decoded_value = decode_bencoded_value(encoded_value, pos);
        std::cout << decoded_value.dump() << std::endl;
    }
    else if (command == "info")
    {
        auto file_path = argv[2];

        std::ifstream file(file_path);
        std::ostringstream buffer;
        buffer << file.rdbuf();

        std::string content = buffer.str();

        size_t pos = 0;
        auto dictionary = decode_dictionary(content, pos);

        Torrent torrent{dictionary};

        std::cout << "Tracker URL: " << torrent.announce << std::endl;
        std::cout << "Length: " << torrent.info.length << std::endl;
        std::cout << "Info Hash: " << torrent.info.sha1() << std::endl;
    }
    else
    {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
