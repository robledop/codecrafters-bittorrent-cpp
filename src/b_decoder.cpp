#include "b_decoder.h"

auto BDecoder::decode_integer(const std::string& encoded_value, size_t& pos) -> int64_t {
    pos++;
    const auto end = encoded_value.find_first_of('e', pos);
    const std::string number_string = encoded_value.substr(pos, end);
    pos = end + 1;
    return std::strtoll(number_string.c_str(), nullptr, 10);
}

auto BDecoder::decode_string_length(const std::string& encoded_value, const size_t pos,
                                    size_t& colon_index) -> int64_t {
    colon_index = encoded_value.find(':', pos);
    const std::string number_string = encoded_value.substr(pos, colon_index);
    const int64_t length = std::strtoll(number_string.c_str(), nullptr, 10);

    return length;
}

auto BDecoder::decode_string(const std::string& encoded_value, size_t& pos) -> std::string {
    size_t colon_index = 0;
    const int64_t length = decode_string_length(encoded_value, pos, colon_index);
    std::string str = encoded_value.substr(colon_index + 1, length);
    pos = colon_index + length + 1;
    return str;
}


auto BDecoder::decode_list(const std::string& encoded_value, size_t& pos) -> json {
    json list = json::array();
    pos++;

    while (encoded_value[pos] != 'e') {
        list.push_back(decode_bencoded_value(encoded_value, pos));
    }

    pos++;

    return list;
}

auto BDecoder::decode_dictionary(const std::string& encoded_value, size_t& pos) -> json {
    json dictionary = json::object();
    pos++;

    while (encoded_value[pos] != 'e') {
        const auto key = decode_bencoded_value(encoded_value, pos);
        const auto value = decode_bencoded_value(encoded_value, pos);
        dictionary[key] = value;
    }

    pos++;

    return dictionary;
}

auto BDecoder::decode_bencoded_value(const std::string& encoded_value, size_t& pos) -> json {
    // string
    // Example: "5:hello" -> "hello"
    if (std::isdigit(encoded_value[pos])) {
        return json(decode_string(encoded_value, pos));
    }

    // Integer
    // Example: "i42e" -> 42
    if (encoded_value[pos] == 'i') {
        return json(decode_integer(encoded_value, pos));
    }

    // List
    // Example: "l5:helloi42ee" -> ["hello", 42] 
    if (encoded_value[pos] == 'l') {
        return decode_list(encoded_value, pos);
    }

    // Dictionary
    // Example: d3:foo3:bar5:helloi52ee -> {"foo":"bar","hello":52}
    if (encoded_value[pos] == 'd') {
        return decode_dictionary(encoded_value, pos);
    }

    throw std::runtime_error("Unhandled encoded value: " + encoded_value);
}

auto BDecoder::encode_bencoded_value(const json& json) -> std::string {
    std::string encoded_value;
    if (json.is_string()) {
        encoded_value = encode_string(json.get<std::string>());
    }
    else if (json.is_number_integer()) {
        encoded_value = encode_integer(json.get<int64_t>());
    }
    else if (json.is_array()) {
        encoded_value = encode_list(json);
    }
    else if (json.is_object()) {
        encoded_value = encode_dictionary(json);
    }
    else {
        throw std::runtime_error("Unhandled JSON type: " + json.dump());
    }
    return encoded_value;
}

auto BDecoder::encode_integer(const int64_t value) -> std::string {
    return "i" + std::to_string(value) + "e";
}

auto BDecoder::encode_string_length(const int64_t value) -> std::string {
    return std::to_string(value) + ":";
}

auto BDecoder::encode_string(const std::string& value) -> std::string {
    return encode_string_length(static_cast<int64_t>(value.length())) + value;
}

auto BDecoder::encode_list(const json& json) -> std::string {
    std::string list = "l";
    for (const auto& item : json) {
        list += encode_bencoded_value(item);
    }
    list += "e";
    return list;
}

auto BDecoder::encode_dictionary(json json) -> std::string {
    std::string dictionary = "d";
    for (const auto& item : json.items()) {
        dictionary += encode_string(item.key());
        dictionary += encode_bencoded_value(item.value());
    }
    dictionary += "e";
    return dictionary;
}
