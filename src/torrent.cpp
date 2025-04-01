#include "torrent.h"

#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <random>
#include <cpr/api.h>
#include <cpr/util.h>
#include <netinet/in.h>

#include "b_decoder.h"
#include "peer.h"
#include "util.h"

Torrent::Torrent(json json_object):
    announce{json_object["announce"]},
    info{Info{json_object["info"]}}, tracker(get_tracker()) {
    std::cout << "Announce: " << announce << std::endl;
    std::cout << "Info: " << std::endl;
    std::cout << "\tName: " << info.name << std::endl;
    std::cout << "\tLength: " << info.length << std::endl;
    std::cout << "\tPiece length: " << info.piece_length << std::endl;

    for (int i = 0; i < get_number_of_pieces(); i++) {
        work_queue.push(i);
    }

    for (auto& peer : tracker.get_peers()) {
        peers_queue.push(peer);
    }
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

auto Torrent::get_tracker() const -> Tracker {
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

    return Tracker{response};
}

auto Torrent::handshake(const std::string& ip, const int port) const -> Peer {
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
        close(sock);
        throw std::runtime_error("Send failed.");
    }

    unsigned char handshake_response[68] = {};
    ssize_t received_bytes = recv(sock, handshake_response, sizeof(handshake_response), 0);
    if (received_bytes < 0) {
        close(sock);
        throw std::runtime_error("Failed to receive handshake response.");
    }

    if (received_bytes != sizeof(handshake_response)) {
        close(sock);
        throw std::runtime_error("Incomplete handshake response.");
    }

    const std::string response(reinterpret_cast<char*>(handshake_response), 68);
    const std::string peer_id = response.substr(48, 20);

    return Peer{peer_id, ip, port, sock};
}

void Torrent::download_piece(int piece_index, const std::string& file_name)  {
    auto [ip, port] = peers_queue_pop();

    // Establish a TCP connection with a peer, and perform a handshake
    auto peer = handshake(ip, port);

    std::cout << "Peer: " << ip << ":" << port << std::endl;
    std::cout << "Peer ID: " << to_hex_string(peer.peer_id) << std::endl;
    std::cout << "Piece Index: " << piece_index << std::endl;

    // Wait for bitfield message
    auto bitfield_message = receive_message(peer.socket);
    if (bitfield_message.message_type != BITFIELD) {
        close(peer.socket);
        throw std::runtime_error("Expected BITFIELD message, but received " + bitfield_message.message_type);
    }

    send_message(peer.socket, INTERESTED);

    // Wait for unchoke message
    if (auto unchoke_message = receive_message(peer.socket); unchoke_message.message_type != UNCHOKE) {
        close(peer.socket);
        throw std::runtime_error("Expected UNCHOKE message, but received " + unchoke_message.message_type);
    }

    std::string buffer{};
    constexpr uint32_t piece_block_size = 16 * 1024;

    uint32_t number_of_pieces = get_number_of_pieces();
    bool is_last_piece = piece_index == number_of_pieces - 1;
    uint32_t file_begin = piece_index * info.piece_length;
    uint32_t actual_piece_length = is_last_piece ? info.length - file_begin : info.piece_length;

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

        uint32_t piece_index_be = htonl(piece_index);
        uint32_t begin_be = htonl(begin);
        uint32_t length_be = htonl(block_length);

        request_payload.append(reinterpret_cast<char*>(&piece_index_be), sizeof(uint32_t));
        request_payload.append(reinterpret_cast<char*>(&begin_be), sizeof(uint32_t));
        request_payload.append(reinterpret_cast<char*>(&length_be), sizeof(uint32_t));

        send_message(peer.socket, REQUEST, request_payload);

        auto piece_message = receive_message(peer.socket);
        if (piece_message.message_type != PIECE) {
            close(peer.socket);
            throw std::runtime_error("Expected PIECE, but received " + piece_message.message_type);
        }

        // Piece payload: piece_index (4 bytes) + begin (4 bytes) + data
        uint32_t received_piece_index = ntohl(*reinterpret_cast<uint32_t*>(piece_message.payload.data()));
        uint32_t received_piece_begin = ntohl(*reinterpret_cast<uint32_t*>(piece_message.payload.data() + 4));

        // Check if it is the same as requested
        if (received_piece_index != piece_index || received_piece_begin != begin) {
            close(peer.socket);
            throw std::runtime_error("Piece index or begin mismatch");
        }

        std::string data = piece_message.payload.substr(8);
        buffer.append(data);
    }

    std::string piece_hash = compute_sha1(buffer);
    std::string expected_piece_hash = info.piece_hashes()[piece_index];

    if (piece_hash != expected_piece_hash) {
        close(peer.socket);
        throw std::runtime_error(
            "Piece hash mismatch. Expected : " + expected_piece_hash +
            ", but received " + piece_hash);
    }

    std::cout << "Piece received successfully" << std::endl;
    close(peer.socket);

    std::string file_path = info.name + ".part" + std::to_string(piece_index);
    if (!file_name.empty()) {
        file_path = file_name;
    }
    std::cout << "Writing to file " << file_path << std::endl;
    std::ofstream file(file_path, std::ios::binary);
    file.write(buffer.c_str(), buffer.length());

    peers_queue_push({ip, port});
}

void Torrent::download_task(const std::string& save_to) {
    while (!work_queue_empty()) {
        const int piece_index = work_queue_pop();
        if (piece_index == -1) {
            break;
        }
        std::cout << "Downloading piece " << piece_index << std::endl;
        try {
            download_piece(piece_index, save_to + ".part" + std::to_string(piece_index));
        }
        catch (const std::exception& e) {
            std::cerr << "Error downloading piece " << piece_index << ": " << e.what() << std::endl;
            work_queue_push(piece_index); // Requeue the piece for retry
        }
    }
}

void Torrent::download(std::string save_to) {
    if (save_to.empty()) {
        save_to = info.name;
    }

    const auto peers = tracker.get_peers();
    for (auto& peer : peers) {
        std::cout << "Peer: " << peer.first << ":" << peer.second << std::endl;
    }
    const auto number_of_workers = std::min(get_number_of_pieces(), static_cast<int>(peers.size()));

    std::cout << "Starting download with " << number_of_workers << " workers." << std::endl;
    std::vector<std::thread> workers{};
    for (int i = 0; i < number_of_workers; i++) {
        std::thread t{&Torrent::download_task, this, save_to};
        workers.push_back(std::move(t));
    }

    for (auto& worker : workers) {
        worker.join();
    }

    std::vector<std::string> files{};
    for (int i = 0; i < get_number_of_pieces(); i++) {
        std::string file_path = save_to + ".part" + std::to_string(i);
        files.push_back(file_path);
    }

    std::ofstream backup("/tmp/torrents/" + info.name, std::ios::binary);
    std::ofstream out(save_to, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Error opening output file: " + save_to);
    }

    if (!backup) {
        throw std::runtime_error("Error opening backup file: /tmp/torrents/" + info.name);
    }

    for (const auto& fileName : files) {
        std::ifstream in(fileName, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Error opening input file: " + fileName);
        }

        out << in.rdbuf();
        in.clear();
        in.seekg(0);
        backup << in.rdbuf();
        in.close();
    }

    out.close();
    backup.close();
    std::cout << "Files have been combined into " << save_to << std::endl;
    std::cout << "Files have been combined into /tmp/torrents/" << info.name << std::endl;
}

auto Torrent::get_number_of_pieces() const -> int {
    return std::ceil(static_cast<double>(info.length) / static_cast<double>(info.piece_length));
}

auto Torrent::work_queue_pop() -> int {
    std::lock_guard<std::mutex> lock(data_mutex);
    if (work_queue.empty()) {
        return -1;
    }
    int piece_index = work_queue.front();
    work_queue.pop();
    return piece_index;
}

auto Torrent::work_queue_empty() const -> bool {
    return work_queue.empty();
}

void Torrent::work_queue_push(const int piece_index) {
    std::lock_guard<std::mutex> lock(data_mutex);
    work_queue.push(piece_index);
}

auto Torrent::peers_queue_pop() -> std::pair<std::string, int> {
    std::lock_guard<std::mutex> lock(data_mutex);
    if (peers_queue.empty()) {
        return {"", -1};
    }

    auto peer = peers_queue.front();
    peers_queue.pop();
    return peer;
}

auto Torrent::peers_queue_empty() const -> bool {
    return peers_queue.empty();
}

void Torrent::peers_queue_push(const std::pair<std::string, int>& peer) {
    std::lock_guard<std::mutex> lock(data_mutex);
    peers_queue.push(peer);
}
