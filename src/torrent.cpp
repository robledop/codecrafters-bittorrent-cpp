#include "torrent.h"

#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <random>
#include <utility>
#include <cpr/api.h>
#include <cpr/util.h>
#include <netinet/in.h>

#include "b_decoder.h"
#include "peer.h"
#include "util.h"

Torrent::Torrent(json json_object):
    announce{json_object["announce"]},
    info{Info{json_object["info"]}},
    info_hash{},
    tracker{0, 0, 0, 0, {}} {
    std::cout << "Announce: " << announce << std::endl;
    std::cout << "Info: " << std::endl;
    std::cout << "\tName: " << info.name << std::endl;
    std::cout << "\tLength: " << info.length << std::endl;
    std::cout << "\tPiece length: " << info.piece_length << std::endl;

    info_hash = info.sha1();

    tracker = get_tracker();

    for (int i = 0; i < get_number_of_pieces(); i++) {
        work_queue.push(i);
    }

    for (auto& peer : tracker.get_peers()) {
        peers_queue.push(peer);
    }
}

Torrent::Torrent(const std::string& announce, const std::string& info_hash)
    : announce{announce},
      info{1, "", 0, ""},
      info_hash{info_hash},
      tracker{0, 0, 0, 0, {}} {
    std::cout << "Announce: " << announce << std::endl;
    std::cout << "Info hash: " << info_hash << std::endl;

    // try {
    //     tracker = get_tracker(hex_to_binary(info_hash));
    //     for (auto& peer : tracker.get_peers()) {
    //         peers_queue.push(peer);
    //     }
    //
    //     auto peer = tracker.get_peers().at(0);
    //     auto handshake = magnet_handshake(peer.first, peer.second, info_hash);
    // }
    // catch (const std::exception& e) {
    //     std::cerr << "Error getting peers: " << e.what() << std::endl;
    // }
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

auto Torrent::parse_magnet_link(const std::string& magnet_link) -> Torrent {
    const auto bith_start = magnet_link.find("xt=urn:btih:") + 12;
    const auto dn_start = magnet_link.find("dn=");
    const auto dn_end = magnet_link.find('&', dn_start + 3);
    const auto tr_start = magnet_link.find("tr=");
    const auto tr_end = magnet_link.find('&', tr_start + 3);
    const auto x_pe_start = magnet_link.find("&x.pe=");
    const auto x_pe_end = magnet_link.find('&', x_pe_start + 6);

    const auto info_hash = magnet_link.substr(bith_start, 40);
    const auto file_name = magnet_link.substr(dn_start + 3, dn_end - dn_start - 3);
    const auto tracker_url = magnet_link.substr(tr_start + 3, tr_end - tr_start - 3);
    const auto peer_address = magnet_link.substr(x_pe_start + 6, x_pe_end - x_pe_start - 6);

    std::cout << "Tracker URL: " << cpr::util::urlDecode(tracker_url) << std::endl;
    std::cout << "Info Hash: " << info_hash << std::endl;

    auto info = Info{0, file_name, 0, ""};
    return Torrent{cpr::util::urlDecode(tracker_url), info_hash};
}

auto Torrent::get_tracker(std::string info_hash) const -> Tracker {
    try {
        if (info_hash.empty()) {
            info_hash = hex_to_binary(info.sha1());
        }
        std::cout << "Getting tracker" << std::endl;
        std::string request_url = announce +
            "?info_hash=" + cpr::util::urlEncode(info_hash) +
            "&peer_id=robledo-pazotto-bitt" +
            "&port=6881" +
            "&uploaded=0" +
            "&downloaded=0" +
            "&left=" + std::to_string(info.length) +
            "&compact=1";

        const cpr::Response r = Get(cpr::Url{request_url});
        size_t pos = 0;
        if (r.status_code != 200) {
            throw std::runtime_error("Failed to get tracker response. Status code: " + std::to_string(r.status_code));
        }
        const auto response = BDecoder::decode_bencoded_value(r.text, pos);

        return Tracker{response};
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting tracker: " << e.what() << std::endl;
        return Tracker{0, 0, 0, 0, {}};
    }
}

auto Torrent::extension_handshake(const std::string& ip, const int port) const -> Peer {
    std::cout << "Handshake: " << ip << ":" << port << std::endl;

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Socket creation failed.");
    }

    sockaddr_in peer_addr = {};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &peer_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address / Address not supported. " + ip);
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&peer_addr), sizeof(peer_addr)) < 0) {
        throw std::runtime_error("Connection failed.");
    }

    unsigned char handshake[68] = {};

    handshake[0] = 19;

    unsigned char extension[8] = {0, 0, 0, 0, 0, 16, 0, 0};

    const auto protocol = "BitTorrent protocol";
    std::memcpy(handshake + 1, protocol, 19);
    std::memcpy(handshake + 20, extension, 8);
    std::memcpy(handshake + 28, hex_to_binary(info_hash).c_str(), 20);
    std::memcpy(handshake + 48, "robledo-pazotto-bitt", 20);


    if (const ssize_t sent_bytes = send(sock, handshake, sizeof(handshake), 0); sent_bytes != sizeof(handshake)) {
        close(sock);
        throw std::runtime_error("Send failed.");
    }

    unsigned char handshake_response[68] = {};
    const ssize_t received_bytes = recv(sock, handshake_response, sizeof(handshake_response), 0);
    if (received_bytes < 0) {
        close(sock);
        throw std::runtime_error("Failed to receive handshake response.");
    }

    if (received_bytes != sizeof(handshake_response)) {
        close(sock);
        throw std::runtime_error("Incomplete handshake response.");
    }

    bool supports_extension = handshake_response[25] & 0x10;
    if (!supports_extension) {
        close(sock);
        throw std::runtime_error("Peer does not support extensions.");
    }

    const std::string response{reinterpret_cast<char*>(handshake_response), 68};
    const std::string peer_id{response.substr(48, 20)};

    return Peer{peer_id, ip, port, sock};
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
        throw std::runtime_error("Invalid address / Address not supported. " + ip);
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

void Torrent::populate_magnet_info(int sock) {
    auto extended_metadata_response = receive_message(sock);

    if (extended_metadata_response.message_type != EXTENDED) {
        close(sock);
        throw std::runtime_error(
            "Expected EXTENDED message, but received " + static_cast<int>(extended_metadata_response.message_type));
    }

    if (extended_metadata_response.payload[0] != MY_METADATA_ID) {
        close(sock);
        throw std::runtime_error("Invalid extended metadata response");
    }

    size_t pos = 0;
    auto header_json = BDecoder::decode_bencoded_value(extended_metadata_response.payload.substr(1), pos);
    if (header_json["msg_type"] != EXTENDED_DATA) {
        close(sock);
        throw std::runtime_error("Invalid extended metadata response");
    }

    auto metadata_json = BDecoder::decode_bencoded_value(extended_metadata_response.payload.substr(1), pos);

    info.name = metadata_json["name"].get<std::string>();
    info.piece_length = metadata_json["piece length"].get<size_t>();
    info.length = metadata_json["length"].get<size_t>();
    info.pieces = metadata_json["pieces"].get<std::string>();

    for (auto& peer : tracker.get_peers()) {
        peers_queue.push(peer);
    }
}

void Torrent::magnet_handshake(int sock) {
    if (auto bitfield_message = receive_message(sock); bitfield_message.message_type != BITFIELD) {
        close(sock);
        throw std::runtime_error("Expected BITFIELD message, but received " + bitfield_message.message_type);
    }

    // Extension message: length (4 bytes) + message ID (1 byte) + extended message ID (1 byte) + payload (variable length)

    json payload_json = json::parse(R"(
    {
      "m": {"ut_metadata": 16}
    }
)"
    );

    const std::string payload = BDecoder::encode_bencoded_value(payload_json);
    std::string extension_handshake;
    extension_handshake.resize(payload.size() + 1);
    extension_handshake[0] = EXTENDED_HANDSHAKE;
    std::memcpy(extension_handshake.data() + 1, payload.c_str(), payload.size());

    send_message(sock, EXTENDED, extension_handshake);

    auto extended_handshake_response = receive_message(sock);
    if (extended_handshake_response.message_type != EXTENDED) {
        close(sock);
        throw std::runtime_error("Expected EXTENDED message, but received " + extended_handshake_response.message_type);
    }

    if (extended_handshake_response.payload[0] != EXTENDED_HANDSHAKE) {
        close(sock);
        throw std::runtime_error("Invalid extended handshake response");
    }

    size_t pos = 0;
    auto extended_message_payload = BDecoder::decode_bencoded_value(extended_handshake_response.payload.substr(1), pos);

    auto m = extended_message_payload["m"];
    if (m.find("ut_metadata") == m.end()) {
        close(sock);
        throw std::runtime_error("Peer does not support metadata");
    }
    int64_t peer_metadata_extension_id = m["ut_metadata"];
    std::cout << "Peer Metadata Extension ID: " << peer_metadata_extension_id << std::endl;

    std::string extended_request;

    json extended_request_payload_json = json::parse(R"({"msg_type": 0, "piece": 0})");

    std::string extended_request_payload = BDecoder::encode_bencoded_value(extended_request_payload_json);
    extended_request.resize(extended_request_payload.size() + 1);
    extended_request[0] = static_cast<char>(peer_metadata_extension_id);
    std::memcpy(extended_request.data() + 1, extended_request_payload.c_str(), extended_request_payload.size());

    send_message(sock, EXTENDED, extended_request);

    // return peer_metadata_extension_id;

    // auto extended_metadata_response = receive_message(sock);
    //
    // if (extended_metadata_response.message_type != EXTENDED) {
    //     close(sock);
    //     throw std::runtime_error(
    //         "Expected EXTENDED message, but received " + static_cast<int>(extended_metadata_response.message_type));
    // }
    //
    // if (extended_metadata_response.payload[0] != MY_METADATA_ID) {
    //     close(sock);
    //     throw std::runtime_error(
    //         "Invalid extended metadata response: " + static_cast<int>(extended_handshake_response.payload[0]));
    // }
    //
    // pos = 0;
    // auto header_json = BDecoder::decode_bencoded_value(extended_metadata_response.payload.substr(1), pos);
    // if (header_json["msg_type"] != EXTENDED_DATA) {
    //     close(sock);
    //     throw std::runtime_error("Invalid extended metadata response");
    // }
    //
    // auto metadata_json = BDecoder::decode_bencoded_value(extended_metadata_response.payload.substr(1), pos);
    //
    // info.name = metadata_json["name"].get<std::string>();
    // info.piece_length = metadata_json["piece length"].get<size_t>();
    // info.length = metadata_json["length"].get<size_t>();
    // info.pieces = metadata_json["pieces"].get<std::string>();
    //
    // return Peer{peer_id, ip, port, sock};
}

void Torrent::download_piece(int piece_index, const std::string& file_name) {
    constexpr uint32_t PIECE_BLOCK_SIZE = 16 * 1024;
    std::string buffer{};

    // TODO: Use a random peer from the queue and check if it is online or not
    auto [ip, port] = peers_queue_pop();

    auto peer = handshake(ip, port);

    std::cout << "Peer: " << ip << ":" << port <<
        ", Peer ID: " << to_hex_string(peer.peer_id) <<
        ", Piece Index: " << piece_index << std::endl;

    // TODO: Actually check if the peer has the piece
    // Wait for bitfield message
    if (auto bitfield_message = receive_message(peer.socket); bitfield_message.message_type != BITFIELD) {
        close(peer.socket);
        throw std::runtime_error("Expected BITFIELD message, but received " + bitfield_message.message_type);
    }

    send_message(peer.socket, INTERESTED);

    // Wait for unchoke message
    if (auto unchoke_message = receive_message(peer.socket); unchoke_message.message_type != UNCHOKE) {
        close(peer.socket);
        throw std::runtime_error("Expected UNCHOKE message, but received " + unchoke_message.message_type);
    }


    uint32_t number_of_pieces = get_number_of_pieces();
    bool is_last_piece = piece_index == number_of_pieces - 1;
    uint32_t file_begin = piece_index * info.piece_length;
    size_t actual_piece_length = is_last_piece ? info.length - file_begin : info.piece_length;

    buffer.resize(actual_piece_length);

    size_t number_of_piece_blocks = std::ceil(
        static_cast<double>(actual_piece_length) / static_cast<double>(PIECE_BLOCK_SIZE));

    size_t remaining_bytes = actual_piece_length;

    // TODO: Limit the number of requests to avoid overwhelming the peer
    for (size_t block = 0; block < number_of_piece_blocks; block++) {
        bool is_last_block = block == number_of_piece_blocks - 1;

        uint32_t begin = block * PIECE_BLOCK_SIZE;
        uint32_t block_length = is_last_block ? actual_piece_length - begin : PIECE_BLOCK_SIZE;

        std::cout << "Piece block #" << block + 1 <<
            " Piece index: " << piece_index <<
            ", Begin: " << begin <<
            ", Block length: " << block_length << std::endl;

        std::string request_payload{};
        request_payload.reserve(12);

        uint32_t piece_index_be = htonl(piece_index);
        uint32_t begin_be = htonl(begin);
        uint32_t length_be = htonl(block_length);

        request_payload.append(reinterpret_cast<char*>(&piece_index_be), sizeof(uint32_t));
        request_payload.append(reinterpret_cast<char*>(&begin_be), sizeof(uint32_t));
        request_payload.append(reinterpret_cast<char*>(&length_be), sizeof(uint32_t));

        send_message(peer.socket, REQUEST, request_payload);
    }

    while (remaining_bytes > 0) {
        auto piece_message = receive_message(peer.socket);
        if (piece_message.message_type != PIECE) {
            close(peer.socket);
            throw std::runtime_error("Expected PIECE, but received " + piece_message.message_type);
        }

        uint32_t received_piece_index = ntohl(*reinterpret_cast<uint32_t*>(piece_message.payload.data()));
        if (received_piece_index != piece_index) {
            throw std::runtime_error("Incorrect piece index");
        }
        uint32_t received_piece_begin = ntohl(*reinterpret_cast<uint32_t*>(piece_message.payload.data() + 4));

        std::string payload = piece_message.payload.substr(8);
        std::ranges::copy(payload, buffer.begin() + received_piece_begin);

        remaining_bytes -= payload.length();
    }

    std::string piece_hash = compute_sha1(buffer);

    if (std::string expected_piece_hash = info.piece_hashes()[piece_index]; piece_hash != expected_piece_hash) {
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
    file.write(buffer.c_str(), static_cast<long>(buffer.length()));

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

    auto number_of_workers = std::min(get_number_of_pieces(), static_cast<int>(peers_queue.size()));

    std::cout << "Starting download with " << number_of_workers << " workers." << std::endl;
    std::vector<std::thread> workers{};
    for (int i = 0; i < number_of_workers; i++) {
        workers.emplace_back(&Torrent::download_task, this, save_to);
    }

    for (auto& worker : workers) {
        worker.join();
    }

    std::vector<std::string> files{};
    for (int i = 0; i < get_number_of_pieces(); i++) {
        std::string file_path = save_to + ".part" + std::to_string(i);
        files.push_back(file_path);
    }

    std::ofstream out(save_to, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Error opening output file: " + save_to);
    }

    for (const auto& fileName : files) {
        std::ifstream in(fileName, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Error opening input file: " + fileName);
        }

        out << in.rdbuf();
        in.close();
    }

    out.close();
    std::cout << "Files have been combined into " << save_to << std::endl;
}

auto Torrent::get_number_of_pieces() const -> int {
    return std::ceil(static_cast<double>(info.length) / static_cast<double>(info.piece_length));
}

auto Torrent::work_queue_pop() -> int {
    std::lock_guard lock{data_mutex};
    if (work_queue.empty()) {
        return -1;
    }
    int piece_index = work_queue.front();
    work_queue.pop();
    return piece_index;
}

auto Torrent::work_queue_empty() -> bool {
    std::lock_guard<std::mutex> lock{data_mutex};
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

auto Torrent::peers_queue_empty() -> bool {
    std::lock_guard<std::mutex> lock(data_mutex);
    return peers_queue.empty();
}

void Torrent::peers_queue_push(const std::pair<std::string, int>& peer) {
    std::lock_guard<std::mutex> lock(data_mutex);
    peers_queue.push(peer);
}
