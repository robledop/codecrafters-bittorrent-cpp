#include <iostream>
#include <string>
#include <fstream>
#include <cpr/cpr.h>

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
        auto torrent = Torrent::parse_magnet_link(magnet_link);
    }
    else if (command == "magnet_handshake") {
        std::string magnet_link = argv[2];
        auto torrent = Torrent::parse_magnet_link(magnet_link);
        torrent.tracker = torrent.get_tracker(hex_to_binary(torrent.info_hash));
        auto [ip, port] = torrent.tracker.get_peers().at(0);
        auto connected_peer = torrent.extension_handshake(ip, port);
        Torrent::magnet_handshake(connected_peer.socket);
        std::cout << "Peer ID: " << to_hex_string(connected_peer.peer_id) << std::endl;
    }
    else if (command == "magnet_info") {
        std::string magnet_link = argv[2];
        auto torrent = Torrent::parse_magnet_link(magnet_link);
        torrent.tracker = torrent.get_tracker(hex_to_binary(torrent.info_hash));
        auto [ip, port] = torrent.tracker.get_peers().at(0);
        auto connected_peer = torrent.extension_handshake(ip, port);
        Torrent::magnet_handshake(connected_peer.socket);
        torrent.populate_magnet_info(connected_peer.socket);

        std::cout << "Tracker URL: " << torrent.announce << std::endl;
        std::cout << "Length: " << torrent.info.length << std::endl;
        std::cout << "Info Hash: " << torrent.info.sha1() << std::endl;
        std::cout << "Piece Length: " << torrent.info.piece_length << std::endl;
        std::cout << "Piece Hashes:" << std::endl;
        for (const auto& piece_hash : torrent.info.piece_hashes()) {
            std::cout << piece_hash << std::endl;
        }
    }
    else if (command == "magnet_download_piece") {
        std::string save_to = argv[3];
        std::string magnet_link = argv[4];
        int piece_index = std::stoi(argv[5]);
        auto torrent = Torrent::parse_magnet_link(magnet_link);
        torrent.tracker = torrent.get_tracker(hex_to_binary(torrent.info_hash));
        auto [ip, port] = torrent.tracker.get_peers().at(0);
        auto connected_peer = torrent.extension_handshake(ip, port);
        Torrent::magnet_handshake(connected_peer.socket);
        torrent.populate_magnet_info(connected_peer.socket);
        torrent.download_piece(piece_index, save_to);
    }
    else if (command == "magnet_download") {
        // TODO: This can be simplified. It's like this because the tests seem to need things to be done separately.
        std::string save_to = argv[3];
        std::string magnet_link = argv[4];
        auto torrent = Torrent::parse_magnet_link(magnet_link);
        torrent.tracker = torrent.get_tracker(hex_to_binary(torrent.info_hash));
        // TODO: Add error handling for when the peer is not available
        auto [ip, port] = torrent.tracker.get_peers().at(0);
        auto connected_peer = torrent.extension_handshake(ip, port);
        Torrent::magnet_handshake(connected_peer.socket);
        torrent.populate_magnet_info(connected_peer.socket);
        torrent.download(save_to);
    }
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
