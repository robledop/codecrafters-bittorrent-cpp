#pragma once
#include <string>

enum MessageType
{
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8,
    EXTENDED = 20,
};

struct Message
{
    MessageType message_type;
    std::string payload;
    ssize_t length;

    Message(const MessageType message_type, const std::string& payload, ssize_t length):
        message_type(message_type),
        payload(payload),
        length(length) {
    }
};
