#include "info.h"

#include "util.h"
#include "lib/sha1.hpp"


Info::Info(json json_object):
    length{json_object["length"]},
    name{json_object["name"]},
    piece_length{json_object["piece length"]},
    pieces{json_object["pieces"]} {
}

auto Info::encode() const -> std::string {
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

auto Info::sha1() const -> std::string {
    return compute_sha1(encode());
}

auto Info::piece_hashes() const -> std::vector<std::string> {
    std::vector<std::string> piece_hashes;
    const size_t length = pieces.length();

    for (size_t i = 0; i < length; i += 20) {
        piece_hashes.push_back(to_hex_string(pieces.substr(i, 20)));
    }

    return piece_hashes;
}
