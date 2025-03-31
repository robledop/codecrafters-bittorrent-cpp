#include <iostream>
#include <string>
#include <fstream>
#include <cpr/cpr.h>
#include <netinet/in.h>
#include <cmath>

#include "b_decoder.h"
#include "handshake.h"
#include "Message.h"
#include "peers.h"
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
        auto peers = torrent.get_tracker();

        std::cout << "Complete: " << peers.get_complete() << std::endl;
        std::cout << "Incomplete: " << peers.get_incomplete() << std::endl;
        std::cout << "Interval: " << peers.get_interval() << std::endl;
        std::cout << "Min interval: " << peers.get_min_interval() << std::endl;
        std::cout << "Peers: " << std::endl;
        for (const auto& [ip, port] : peers.get_peers()) {
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
        const auto file_path = argv[3];
        const auto torrent_file = argv[4];
        const auto piece_index = std::stoi(argv[5]);

        std::cerr << "Save to: " << file_path <<
            ", Torrent file: " << torrent_file <<
            ", Piece index: " << piece_index
            << std::endl;

        // Read the torrent file to get the tracker URL
        const auto torrent = Torrent::parse_torrent_file(torrent_file);

        // Perform the tracker GET request to get a list of peers
        const auto peers = torrent.get_tracker();
        auto [ip, port] = peers.get_peers().at(0);

        // Establish a TCP connection with a peer, and perform a handshake
        auto handshake = torrent.handshake(ip, port);

        std::cout << "Peer: " << ip << ":" << port << std::endl;
        std::cout << "Peer ID: " << to_hex_string(handshake.peer_id) << std::endl;
        std::cout << "Piece Index: " << piece_index << std::endl;

        // Exchange multiple peer messages to download the file

        // Wait for bitfield message
        auto bitfield_message = receive_message(handshake.socket);
        if (bitfield_message.message_type != BITFIELD) {
            close(handshake.socket);
            throw std::runtime_error("Expected BITFIELD message, but received " + bitfield_message.message_type);
        }

        send_message(handshake.socket, INTERESTED);

        // Wait for unchoke message
        if (auto unchoke_message = receive_message(handshake.socket); unchoke_message.message_type != UNCHOKE) {
            close(handshake.socket);
            throw std::runtime_error("Expected UNCHOKE message, but received " + unchoke_message.message_type);
        }

        std::string buffer{};
        constexpr uint32_t piece_block_size = 16 * 1024;

        uint32_t number_of_pieces = std::ceil(
            static_cast<double>(torrent.info.length) / static_cast<double>(torrent.info.piece_length));
        bool is_last_piece = piece_index == number_of_pieces - 1;
        uint32_t file_begin = piece_index * torrent.info.piece_length;
        uint32_t actual_piece_length = is_last_piece ? torrent.info.length - file_begin : torrent.info.piece_length;

        buffer.reserve(actual_piece_length);

        size_t number_of_piece_blocks = std::ceil(
            static_cast<double>(actual_piece_length) / static_cast<double>(piece_block_size));

        std::cout << "Requesting all " << number_of_piece_blocks << " piece blocks." << std::endl;

        // Request all blocks
        for (size_t block = 0; block < number_of_piece_blocks; block++) {
            bool is_last_block = block == number_of_piece_blocks - 1;

            uint32_t begin = block * piece_block_size;
            uint32_t block_length = is_last_block ? actual_piece_length - begin : piece_block_size;

            if (is_last_block) {
                std::cout << "Last ";
            }
            std::cout << "Piece block #" << block + 1 <<
                " Piece index: " << piece_index <<
                " Begin: " << begin <<
                " Block length: " << block_length << std::endl;

            std::string request_payload{};
            request_payload.reserve(12);

            request_payload += (piece_index >> 24) & 0xFF;
            request_payload += (piece_index >> 16) & 0xFF;
            request_payload += (piece_index >> 8) & 0xFF;
            request_payload += piece_index & 0xFF;

            request_payload += (begin >> 24) & 0xFF;
            request_payload += (begin >> 16) & 0xFF;
            request_payload += (begin >> 8) & 0xFF;
            request_payload += begin & 0xFF;

            request_payload += (block_length >> 24) & 0xFF;
            request_payload += (block_length >> 16) & 0xFF;
            request_payload += (block_length >> 8) & 0xFF;
            request_payload += block_length & 0xFF;

            send_message(handshake.socket, REQUEST, request_payload);

            auto piece_message = receive_message(handshake.socket);
            if (piece_message.message_type != PIECE) {
                close(handshake.socket);
                throw std::runtime_error("Expected PIECE, but received " + piece_message.message_type);
            }

            // Piece payload: piece_index (4 bytes) + begin (4 bytes) + data
            uint32_t received_piece_index = ntohl(*reinterpret_cast<uint32_t*>(piece_message.payload.data()));
            uint32_t received_piece_begin = ntohl(*reinterpret_cast<uint32_t*>(piece_message.payload.data() + 4));

            // Check if it is the same as requested
            if (received_piece_index != piece_index || received_piece_begin != begin) {
                close(handshake.socket);
                throw std::runtime_error("Piece index or begin mismatch");
            }

            std::string data = piece_message.payload.substr(8);
            buffer.append(data);
        }

        std::string piece_hash = compute_sha1(buffer);
        std::string expected_piece_hash = torrent.info.piece_hashes()[piece_index];

        if (piece_hash != expected_piece_hash) {
            close(handshake.socket);
            throw std::runtime_error(
                "Piece hash mismatch. Expected : " + expected_piece_hash +
                ", but received " + piece_hash);
        }

        std::cout << "Piece received successfully" << std::endl;
        close(handshake.socket);

        std::cout << "Writing to file" << std::endl;
        std::ofstream file(file_path, std::ios::binary);
        file.write(buffer.c_str(), buffer.length());
    }
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
