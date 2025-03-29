#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

long long int extract_number(const std::string& encoded_value)
{
    const std::string number_string = encoded_value.substr(1, encoded_value.size() - 2);
    return std::strtoll(number_string.c_str(), nullptr, 10);
}

int64_t extract_string_length(const std::string& encoded_value, size_t pos, size_t& colon_index)
{
    colon_index = encoded_value.find(':');
    const std::string number_string = encoded_value.substr(pos, colon_index);
    const int64_t length = std::strtoll(number_string.c_str(), nullptr, 10);

    return length;
}

std::string extract_string(const std::string& encoded_value, size_t pos)
{
    size_t colon_index = 0;
    const int64_t length = extract_string_length(encoded_value, pos, colon_index);
    std::string str = encoded_value.substr(colon_index + 1, length);
    return str;
}

json extract_list(const std::string& encoded_value, size_t& pos)
{
    json list = json::array();

    pos++;

    while (pos < encoded_value.size())
    {
        if (std::isdigit(encoded_value[pos]))
        {
            size_t colon_index = 0;
            const int64_t length = extract_string_length(encoded_value, pos, colon_index);
            std::string str = encoded_value.substr(colon_index + 1, length);
            list.push_back(str);
            pos += colon_index + str.size();
        }
        else if (encoded_value[pos] == 'i')
        {
            const auto end = encoded_value.find_first_of('e', pos);
            auto number = extract_number(encoded_value.substr(pos, end));
            list.push_back(number);
            pos = end + 1;
        }
        else if (encoded_value[pos] == 'l')
        {
            list.push_back(extract_list(encoded_value, pos));
        }
        else if (encoded_value[pos] == 'e')
        {
            // End of this list
            pos++;
            break;
        }
        else
        {
            break;
        }
    }


    return list;
}

json decode_bencoded_value(const std::string& encoded_value)
{
    // string
    if (std::isdigit(encoded_value[0]))
    {
        // Example: "5:hello" -> "hello"
        if (encoded_value.contains(':'))
        {
            return json(extract_string(encoded_value, 0));
        }

        throw std::runtime_error("Invalid encoded value: " + encoded_value);
    }

    // Integer
    // Example: "i42e" -> 42
    if (encoded_value.starts_with('i') && encoded_value.ends_with('e'))
    {
        return json(extract_number(encoded_value));
    }

    // List
    // Example: "l5:helloi42ee" -> ["hello", 42] 
    if (encoded_value.starts_with('l') && encoded_value.ends_with('e'))
    {
        size_t pos = 0;
        return extract_list(encoded_value, pos);
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
