#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded_value)
{
    if (std::isdigit(encoded_value[0]))
    {
        // Example: "5:hello" -> "hello"
        if (const size_t colon_index = encoded_value.find(':'); colon_index != std::string::npos)
        {
            const std::string number_string = encoded_value.substr(0, colon_index);
            const int64_t number = std::strtoll(number_string.c_str(), nullptr, 10);
            std::string str = encoded_value.substr(colon_index + 1, number);
            return json(str);
        }

        throw std::runtime_error("Invalid encoded value: " + encoded_value);
    }

    // Example: "i42e" -> 42
    if (encoded_value.starts_with('i') && encoded_value.ends_with('e'))
    {
        const std::string number_string = encoded_value.substr(1, encoded_value.size() - 2);
        return json(std::strtoll(number_string.c_str(), nullptr, 10));
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
