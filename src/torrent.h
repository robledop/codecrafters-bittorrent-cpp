#pragma once
#include <queue>
#include <string>
#include <random>

#include "info.h"
#include "peer.h"
#include "tracker.h"

enum ExtendedMessageType
{
    EXTENDED_REQUEST = 0,
    EXTENDED_DATA = 1,
    EXTENDED_REJECT = 2,
};

constexpr int EXTENDED_HANDSHAKE = 0;
constexpr int MY_METADATA_ID = 16;

struct Torrent
{
    std::string announce;
    Info info;
    std::string info_hash;
    Tracker tracker;

    explicit Torrent(json json_object);
    Torrent(const std::string& announce, const std::string& info_hash);
    static auto parse_torrent_file(char* path) -> Torrent;
    static auto parse_magnet_link(const std::string& magnet_link) -> Torrent;
    void populate_magnet_info(int sock);
    void download_piece(int piece_index, const std::string& file_name = "");
    void download_task(const std::string& save_to);
    void download(std::string save_to = "");
    [[nodiscard]] auto get_tracker(std::string info_hash = "") const -> Tracker;
    [[nodiscard]] auto extension_handshake(const std::string& ip, int port) const -> Peer;
    [[nodiscard]] auto handshake(const std::string& ip, int port) const -> Peer;
    static auto magnet_handshake(int sock) -> void;

private:
    [[nodiscard]] auto get_number_of_pieces() const -> int;
    [[nodiscard]] auto work_queue_pop() -> int;
    [[nodiscard]] auto work_queue_empty() -> bool;
    [[nodiscard]] auto peers_queue_pop() -> std::pair<std::string, int>;
    [[nodiscard]] auto peers_queue_empty() -> bool;
    void work_queue_push(int piece_index);
    void peers_queue_push(const std::pair<std::string, int>& peer);
    std::queue<int> work_queue{};
    std::queue<std::pair<std::string, int>> peers_queue{};
    std::mutex data_mutex;
};
