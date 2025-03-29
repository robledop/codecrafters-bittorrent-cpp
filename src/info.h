#pragma once
#include <string>

#include "lib/nlohmann/json.hpp"
using json = nlohmann::json;

struct Info
{
    size_t length;
    std::string name;
    size_t piece_length;
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
};
