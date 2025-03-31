#pragma once
#include <string>


struct Handshake
{
    std::string peer_id;
    std::string peer_ip;
    int peer_port;
    int socket;

    Handshake(const std::string& peer_id, const std::string& peer_ip, int peer_port, int socket): peer_id{peer_id},
        peer_ip{peer_ip}, peer_port{peer_port}, socket{socket}
    {
    }
};
