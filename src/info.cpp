#include "info.h"
#include "lib/sha1.hpp"

std::string to_hex_string(const std::string& data)
{
    std::ostringstream oss;
    oss << std::hex;
    for (const unsigned char c : data)
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    return oss.str();
}

std::string hex_to_binary(const std::string& hex)
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

auto Info::encode() const -> std::string
{
    // TODO: generalize this
    std::string encoded_dictionary;
    encoded_dictionary.append("d6:length");
    encoded_dictionary.append("i");
    encoded_dictionary.append(std::to_string(length));
    encoded_dictionary.append("e");
    encoded_dictionary.append("4:name");
    encoded_dictionary.append(std::to_string(name.length()));
    encoded_dictionary.append(":");
    encoded_dictionary.append(name);
    encoded_dictionary.append("12:piece length");
    encoded_dictionary.append("i");
    encoded_dictionary.append(std::to_string(piece_length));
    encoded_dictionary.append("e");
    encoded_dictionary.append("6:pieces");
    encoded_dictionary.append(std::to_string(pieces.length()));
    encoded_dictionary.append(":");
    encoded_dictionary.append(pieces);
    encoded_dictionary.append("e");

    return encoded_dictionary;
}

std::string Info::sha1() const
{
    SHA1 sha1;
    sha1.update(encode());
    std::string hash = sha1.final();
    return hash;
}

std::vector<std::string> Info::piece_hashes() const
{
    std::vector<std::string> piece_hashes;
    const size_t length = pieces.length();

    for (size_t i = 0; i < length; i += 20)
    {
        piece_hashes.push_back(to_hex_string(pieces.substr(i, 20)));
    }
    
    return piece_hashes;
}
