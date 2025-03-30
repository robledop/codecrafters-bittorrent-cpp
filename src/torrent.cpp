#include "torrent.h"

#include <fstream>

#include "b_decoder.h"

Torrent::Torrent(json json_object):
    announce{json_object["announce"]},
    info{Info{json_object["info"]}}
{
}

auto Torrent::parse_torrent_file(char* path) -> Torrent
{
    std::ifstream file(path);
    std::ostringstream buffer;
    buffer << file.rdbuf();

    std::string content = buffer.str();

    size_t pos = 0;
    auto dictionary = BDecoder::decode_bencoded_value(content, pos);

    return Torrent{dictionary};
}
