#pragma once
#include <string>

#include "info.h"

struct Torrent
{
    std::string announce;
    Info info;

    explicit Torrent(json json_object):
        announce{json_object["announce"]},
        info{Info{json_object["info"]}}
    {
    }
};
