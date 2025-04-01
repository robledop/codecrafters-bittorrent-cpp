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
    void download_piece(int piece_index, const std::string& file_name = "");
    void download_task(const std::string& save_to);
    void download(std::string save_to = "");

private:
    [[nodiscard]] auto get_number_of_pieces() const -> int;
    [[nodiscard]] auto work_queue_pop() -> int;
    [[nodiscard]] auto work_queue_empty() const -> bool;
    void work_queue_push(int piece_index);
    [[nodiscard]] auto peers_queue_pop() -> std::pair<std::string, int>;
    [[nodiscard]] auto peers_queue_empty() const -> bool;
    void peers_queue_push(const std::pair<std::string, int>& peer);
    std::queue<int> work_queue{};
    std::queue<std::pair<std::string, int>> peers_queue{};
    std::mutex data_mutex;
};
