#pragma once
#include <queue>
#include <string>
#include <random>

#include "info.h"
#include "tracker.h"

class Peer;

struct Torrent
{
    std::string announce;
    Info info;

    explicit Torrent(json json_object);
    static auto parse_torrent_file(char* path) -> Torrent;
    [[nodiscard]] auto get_tracker() const -> Tracker;
    [[nodiscard]] auto handshake(const std::string& ip, int port) const -> Peer;
    void download_piece(int piece_index, const std::string& file_name = "") const;
    void download_task(const std::string& save_to);
    void download(std::string save_to = "");

private:
    [[nodiscard]] auto get_number_of_pieces() const -> int;
    [[nodiscard]] auto work_queue_pop() -> int;
    [[nodiscard]] auto work_queue_empty() const -> bool;
    [[nodiscard]] auto work_queue_size() const -> size_t;
    static auto get_random_peer(std::vector<std::pair<std::string, int>> peers) -> std::pair<std::string, int>;
    void work_queue_push(int piece_index);
    std::queue<int> work_queue{};
    std::mutex data_mutex;
};
