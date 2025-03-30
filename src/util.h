#pragma once
#include <iomanip>
#include <sstream>
#include <string>

inline auto to_hex_string(const std::string& data) -> std::string
{
    std::ostringstream oss;
    oss << std::hex;
    for (const unsigned char c : data)
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    return oss.str();
}

inline auto hex_to_binary(const std::string& hex) -> std::string
{
    std::string binary;
    binary.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        const char byte = static_cast<char>(std::strtol(byteString.c_str(), nullptr, 16));
        binary.push_back(byte);
    }
    return binary;
}
