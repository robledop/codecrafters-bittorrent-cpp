#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded_value, size_t& pos);

long long int decode_integer(const std::string& encoded_value, size_t& pos)
{
    const auto end = encoded_value.find_first_of('e', pos);
    const std::string number_string = encoded_value.substr(pos, end);
    pos = end;
    return std::strtoll(number_string.c_str(), nullptr, 10);
}

int64_t decode_string_length(const std::string& encoded_value, const size_t pos, size_t& colon_index)
{
    colon_index = encoded_value.find(':');
    const std::string number_string = encoded_value.substr(pos, colon_index);
    const int64_t length = std::strtoll(number_string.c_str(), nullptr, 10);

    return length;
}

std::string decode_string(const std::string& encoded_value, size_t& pos)
{
    size_t colon_index = 0;
    const int64_t length = decode_string_length(encoded_value, pos, colon_index);
    std::string str = encoded_value.substr(colon_index + 1, length);
    pos = colon_index + length;
    return str;
}

json decode_list(const std::string& encoded_value, size_t& pos)
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

json decode_bencoded_value(const std::string& encoded_value, size_t& pos)
{
    // string
    if (std::isdigit(encoded_value[0]))
    {
        // Example: "5:hello" -> "hello"
        if (encoded_value.contains(':'))
        {
            return json(decode_string(encoded_value, pos));
        }

        throw std::runtime_error("Invalid encoded value: " + encoded_value);
    }

    // Integer
    // Example: "i42e" -> 42
    if (encoded_value.starts_with('i') && encoded_value.ends_with('e'))
    {
        return json(decode_integer(encoded_value, pos));
    }

    // List
    // Example: "l5:helloi42ee" -> ["hello", 42] 
    if (encoded_value.starts_with('l') && encoded_value.ends_with('e'))
    {
        return decode_list(encoded_value, pos);
    }

    throw std::runtime_error("Unhandled encoded value: " + encoded_value);
}


int main(int argc, char* argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    const std::string command = argv[1];

    if (command == "decode")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        const std::string encoded_value = argv[2];
        const json decoded_value = decode_bencoded_value(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    }
    else
    {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
