#pragma once
#include <string>

#include "info.h"
#include "peers.h"

class Handshake;

struct Torrent
{
    std::string announce;
    Info info;

    explicit Torrent(json json_object);
    static auto parse_torrent_file(char* path) -> Torrent;
    [[nodiscard]] auto get_tracker() const -> Peers;
    auto handshake(const std::string& ip, int port) const -> Handshake;
};
