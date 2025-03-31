#include "torrent.h"

#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <cpr/api.h>
#include <cpr/util.h>
#include <netinet/in.h>

#include "b_decoder.h"
#include "handshake.h"
#include "util.h"

Torrent::Torrent(json json_object):
    announce{json_object["announce"]},
    info{Info{json_object["info"]}} {
    std::cout << "Announce: " << announce << std::endl;
    std::cout << "Info: " << std::endl;
    std::cout << "\tName: " << info.name << std::endl;
    std::cout << "\tLength: " << info.length << std::endl;
    std::cout << "\tPiece length: " << info.piece_length << std::endl;
}

auto Torrent::parse_torrent_file(char* path) -> Torrent {
    std::cout << "Parsing torrent file: " << path << std::endl;
    std::ifstream file(path);
    std::ostringstream buffer;
    buffer << file.rdbuf();

    std::string content = buffer.str();

    size_t pos = 0;
    auto dictionary = BDecoder::decode_bencoded_value(content, pos);

    return Torrent{dictionary};
}

auto Torrent::get_tracker() const -> Peers {
    std::cout << "Getting tracker" << std::endl;
    const std::string request_url = announce +
        "?info_hash=" + cpr::util::urlEncode(hex_to_binary(info.sha1())) +
        "&peer_id=robledo-pazotto-bitt" +
        "&port=6881" +
        "&uploaded=0" +
        "&downloaded=0" +
        "&left=" + std::to_string(info.length) +
        "&compact=1";

    cpr::Response r = Get(cpr::Url{request_url});
    size_t pos = 0;
    const auto response = BDecoder::decode_bencoded_value(r.text, pos);
    return Peers{response};
}

auto Torrent::handshake(const std::string& ip, const int port) const -> Handshake {
    std::cout << "Handshake: " << ip << ":" << port << std::endl;
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Socket creation failed.");
    }

    sockaddr_in peer_addr = {};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &peer_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address / Address not supported.");
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&peer_addr), sizeof(peer_addr)) < 0) {
        throw std::runtime_error("Connection failed.");
    }

    unsigned char handshake[68] = {};

    handshake[0] = 19;

    const auto protocol = "BitTorrent protocol";
    std::memcpy(handshake + 1, protocol, 19);

    std::memcpy(handshake + 28, hex_to_binary(info.sha1()).c_str(), 20);
    std::memcpy(handshake + 48, "robledo-pazotto-bitt", 20);

    if (const ssize_t sent_bytes = send(sock, handshake, sizeof(handshake), 0); sent_bytes != sizeof(handshake)) {
        throw std::runtime_error("Send failed.");
    }

    unsigned char handshake_response[68] = {};
    ssize_t received_bytes = recv(sock, handshake_response, sizeof(handshake_response), 0);
    if (received_bytes < 0) {
        close(sock);
        throw std::runtime_error("Failed to receive handshake response.");
    }

    if (received_bytes != sizeof(handshake_response)) {
        throw std::runtime_error("Incomplete handshake response.");
    }

    const std::string response(reinterpret_cast<char*>(handshake_response), 68);
    const std::string peer_id = response.substr(48, 20);

    return Handshake{peer_id, ip, port, sock};
}
