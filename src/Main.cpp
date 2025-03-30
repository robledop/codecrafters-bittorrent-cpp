#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <cpr/cpr.h>

#include "b_decoder.h"
#include "peers.h"
#include "torrent.h"
#include "util.h"
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

auto main(int argc, char* argv[]) -> int
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    if (const std::string command = argv[1]; command == "decode")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        const std::string encoded_value = argv[2];
        size_t pos = 0;
        const json decoded_value = BDecoder::decode_bencoded_value(encoded_value, pos);
        std::cout << decoded_value.dump() << std::endl;
    }
    else if (command == "info")
    {
        auto file_path = argv[2];

        Torrent torrent = Torrent::parse_torrent_file(file_path);

        std::cout << "Tracker URL: " << torrent.announce << std::endl;
        std::cout << "Length: " << torrent.info.length << std::endl;
        std::cout << "Info Hash: " << torrent.info.sha1() << std::endl;
        std::cout << "Piece Length: " << torrent.info.piece_length << std::endl;
        std::cout << "Piece Hashes" << std::endl;
        for (const auto& piece_hash : torrent.info.piece_hashes())
        {
            std::cout << piece_hash << std::endl;
        }
    }
    else if (command == "peers")
    {
        const auto file_path = argv[2];
        const Torrent torrent = Torrent::parse_torrent_file(file_path);

        const std::string request_url = torrent.announce +
            "?info_hash=" + cpr::util::urlEncode(hex_to_binary(torrent.info.sha1())) +
            "&peer_id=robledo-pazotto-bitt" +
            "&port=6881" +
            "&uploaded=0" +
            "&downloaded=0" +
            "&left=" + std::to_string(torrent.info.length) +
            "&compact=1";

        cpr::Response r = Get(cpr::Url{request_url});
        size_t pos = 0;
        std::cerr << r.text << std::endl;
        auto response = BDecoder::decode_bencoded_value(r.text, pos);
        auto peers = Peers{response};

        std::cout << "Complete: " << peers.get_complete() << std::endl;
        std::cout << "Incomplete: " << peers.get_incomplete() << std::endl;
        std::cout << "Interval: " << peers.get_interval() << std::endl;
        std::cout << "Min interval: " << peers.get_min_interval() << std::endl;
        std::cout << "Peers: " << std::endl;
        for (const auto& [ip, port] : peers.get_peers())
        {
            std::cout << ip << ":" << port << std::endl;
        }
    }
    else
    {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
