#pragma once
#include <iomanip>
#include <sstream>
#include <string>

#include "lib/sha1.hpp"

inline auto to_hex_string(const std::string& data) -> std::string {
    std::ostringstream oss;
    oss << std::hex;
    for (const unsigned char c : data)
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    return oss.str();
}

inline auto hex_to_binary(const std::string& hex) -> std::string {
    std::string binary;
    binary.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        const char byte = static_cast<char>(std::strtol(byteString.c_str(), nullptr, 16));
        binary.push_back(byte);
    }
    return binary;
}

inline auto compute_sha1(const std::string& data) -> std::string {
    SHA1 sha1;
    sha1.update(data);
    std::string hash = sha1.final();
    return hash;
}

inline void send_message(const int sockfd, const uint8_t message_type, const std::string& payload = "") {
    const uint32_t length = payload.length() + 1;
    const uint32_t length_be = htonl(length);

    if (send(sockfd, &length_be, 4, 0) != 4) {
        close(sockfd);
        throw std::runtime_error("Failed to send message length");
    }

    if (message_type == CHOKE && payload.empty()) {
        std::cout << "Keep-alive message sent: 4 bytes." << std::endl;
        return;
    }

    if (send(sockfd, &message_type, 1, 0) != 1) {
        close(sockfd);
        throw std::runtime_error("Failed to send message type");
    }

    if (!payload.empty()) {
        if (send(sockfd, payload.c_str(), payload.length(), 0) != payload.length()) {
            close(sockfd);
            throw std::runtime_error("Failed to send message payload");
        }
    }
}

inline auto receive_message(const int sockfd) -> Message {
    uint32_t length_be;
    int bytes_received = recv(sockfd, &length_be, 4, 0);
    if (bytes_received != 4) {
        throw std::runtime_error("Failed to receive message length");
    }

    const uint32_t length = ntohl(length_be);
    if (length == 0) {
        std::cout << "Keep-alive message received: 4 bytes." << std::endl;
        return {CHOKE, ""};
    }

    std::vector<char> buffer(length);

    int total_received = 0;
    while (total_received < length) {
        bytes_received = recv(sockfd, buffer.data() + total_received, length - total_received, 0);
        if (bytes_received <= 0) {
            close(sockfd);
            throw std::runtime_error("Failed to receive message length: " + std::to_string(bytes_received));
        }
        total_received += bytes_received;
    }

    auto message_type = static_cast<MessageType>(buffer[0]);
    std::string payload{buffer.data() + 1, length - 1};
    return {message_type, payload};
}
