#include "info.h"
#include "lib/sha1.hpp"

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

