#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

class Peers
{
    int64_t interval;
    int64_t min_interval;
    int64_t complete;
    int64_t incomplete;
    std::vector<std::pair<std::string, int>> peers;

public:
    explicit Peers(json response);
    [[nodiscard]] auto get_interval() const -> int64_t;
    [[nodiscard]] auto get_min_interval() const -> int64_t;
    [[nodiscard]] auto get_complete() const -> int64_t;
    [[nodiscard]] auto get_incomplete() const -> int64_t;
    [[nodiscard]] auto get_peers() const -> std::vector<std::pair<std::string, int>>;
};
