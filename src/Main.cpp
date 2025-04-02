#include <iostream>
#include <string>
#include <fstream>
#include <cpr/cpr.h>
#include <netinet/in.h>
#include <cmath>

#include "b_decoder.h"
#include "peer.h"
#include "tracker.h"
#include "torrent.h"
#include "util.h"
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

auto main(int argc, char* argv[]) -> int {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    if (const std::string command = argv[1]; command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        const std::string encoded_value = argv[2];
        size_t pos = 0;
        const json decoded_value = BDecoder::decode_bencoded_value(encoded_value, pos);
        std::cout << decoded_value.dump() << std::endl;
    }
    else if (command == "info") {
        auto file_path = argv[2];

        Torrent torrent = Torrent::parse_torrent_file(file_path);

        std::cout << "Tracker URL: " << torrent.announce << std::endl;
        std::cout << "Length: " << torrent.info.length << std::endl;
        std::cout << "Info Hash: " << torrent.info.sha1() << std::endl;
        std::cout << "Piece Length: " << torrent.info.piece_length << std::endl;
        std::cout << "Piece Hashes" << std::endl;
        for (const auto& piece_hash : torrent.info.piece_hashes()) {
            std::cout << piece_hash << std::endl;
        }
    }
    else if (command == "peers") {
        const auto file_path = argv[2];
        const Torrent torrent = Torrent::parse_torrent_file(file_path);
        auto tracker = torrent.get_tracker();

        std::cout << "Complete: " << tracker.get_complete() << std::endl;
        std::cout << "Incomplete: " << tracker.get_incomplete() << std::endl;
        std::cout << "Interval: " << tracker.get_interval() << std::endl;
        std::cout << "Min interval: " << tracker.get_min_interval() << std::endl;
        std::cout << "Peers: " << std::endl;
        for (const auto& [ip, port] : tracker.get_peers()) {
            std::cout << ip << ":" << port << std::endl;
        }
    }
    else if (command == "handshake") {
        const auto file_path = argv[2];
        const Torrent torrent = Torrent::parse_torrent_file(file_path);

        // <peer_ip>:<peer_port>
        const std::string peer = argv[3];
        std::cerr << peer << std::endl;

        std::string ip = peer.substr(0, peer.find(':'));
        std::string port_str = peer.substr(peer.find(':') + 1);
        int port = std::stoi(port_str);

        auto handshake = torrent.handshake(ip, port);

        std::cout << "Peer ID: " << to_hex_string(handshake.peer_id) << std::endl;
    }
    else if (command == "download_piece") {
        const auto save_to = argv[3];
        const auto torrent_file = argv[4];
        const auto piece_index = std::stoi(argv[5]);

        std::cerr << "Save to: " << save_to <<
            ", Torrent file: " << torrent_file <<
            ", Piece index: " << piece_index
            << std::endl;

        // Read the torrent file to get the tracker URL
        auto torrent = Torrent::parse_torrent_file(torrent_file);
        torrent.download_piece(piece_index, save_to);
    }
    else if (command == "download") {
        const auto save_to = argv[3];
        const auto torrent_file = argv[4];
        auto torrent = Torrent::parse_torrent_file(torrent_file);
        torrent.download(save_to);
    }
    else if (command == "magnet_parse") {
        std::string magnet_link = argv[2];
        // v1: magnet:?xt=urn:btih:<info-hash>&dn=<name>&tr=<tracker-url>&x.pe=<peer-address>
        auto bith_start = magnet_link.find("xt=urn:btih:") + 12;
        auto dn_start = magnet_link.find("dn=");
        auto dn_end = magnet_link.find('&', dn_start + 3);
        auto tr_start = magnet_link.find("tr=");
        auto tr_end = magnet_link.find('&', tr_start + 3);
        auto x_pe_start = magnet_link.find("&x.pe=");
        auto x_pe_end = magnet_link.find('&', x_pe_start + 6);

        auto info_hash = magnet_link.substr(bith_start, 40);
        auto file_name = magnet_link.substr(dn_start + 3, dn_end - dn_start - 3);
        auto tracker_url = magnet_link.substr(tr_start + 3, tr_end - tr_start - 3);
        auto peer_address = magnet_link.substr(x_pe_start + 6, x_pe_end - x_pe_start - 6);

        std::cout << "Info Hash: " << info_hash << std::endl;
        if (tr_start != std::string::npos) {
            std::cout << "Tracker URL: " << cpr::util::urlDecode(tracker_url) << std::endl;
        }

        if (dn_start != std::string::npos) {
            std::cout << "File Name: " << file_name << std::endl;
        }

        if (x_pe_start != std::string::npos) {
            std::cout << "Peer Address: " << peer_address << std::endl;
        }
    }
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
