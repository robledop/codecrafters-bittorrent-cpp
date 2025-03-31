#include "peers.h"

Peers::Peers(json response): interval{response["interval"]},
                             min_interval{response["min interval"]},
                             complete{response["complete"]},
                             incomplete{response["incomplete"]}
{
    const auto peers_string = response["peers"].get<std::string>();

    for (size_t i = 0; i < peers_string.length(); i += 6)
    {
        std::string ip = std::to_string(static_cast<unsigned char>(peers_string[i])) + "." +
            std::to_string(static_cast<unsigned char>(peers_string[i + 1])) + "." +
            std::to_string(static_cast<unsigned char>(peers_string[i + 2])) + "." +
            std::to_string(static_cast<unsigned char>(peers_string[i + 3]));
        uint16_t port = (static_cast<uint16_t>(static_cast<unsigned char>(peers_string[i + 4]) << 8)) | static_cast<
            uint16_t>(static_cast<unsigned char>(peers_string[i + 5]));

        peers.emplace_back(ip, port);
    }
}

auto Peers::get_interval() const -> int64_t
{
    return interval;
}

auto Peers::get_min_interval() const -> int64_t
{
    return min_interval;
}

auto Peers::get_complete() const -> int64_t
{
    return complete;
}

auto Peers::get_incomplete() const -> int64_t
{
    return incomplete;
}

auto Peers::get_peers() const -> std::vector<std::pair<std::string, int>>
{
    return peers;
}
