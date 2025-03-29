#pragma once
#include <string>

#include "lib/nlohmann/json.hpp"
using json = nlohmann::json;

struct Info
{
    size_t length;
    std::string name;
    size_t piece_length;

    // pieces maps to a string whose length is a multiple of 20.
    // It is to be subdivided into strings of length 20, each of which
    // is the SHA1 hash of the piece at the corresponding index.
    std::string pieces;

    explicit Info(json json_object):
        length{json_object["length"]},
        name{json_object["name"]},
        piece_length{json_object["piece length"]},
        pieces{json_object["pieces"]}
    {
    }

    [[nodiscard]] std::string encode() const;
    [[nodiscard]] std::string sha1() const;
    [[nodiscard]] std::vector<std::string> piece_hashes() const;
};
