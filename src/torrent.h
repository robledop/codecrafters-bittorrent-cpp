#pragma once
#include <string>

#include "info.h"

struct Torrent
{
    std::string announce;
    Info info;

    explicit Torrent(json json_object);
    static auto parse_torrent_file(char* path) ->Torrent;
};
