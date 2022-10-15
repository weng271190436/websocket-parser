#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <algorithm>

struct Frame {
    bool fin;
    bool rsv1;
    bool rsv2;
    bool rsv3;
    uint8_t opcode;
    bool mask;
    uint64_t payload_len;
    uint8_t mask_key_1;
    uint8_t mask_key_2;
    uint8_t mask_key_3;
    uint8_t mask_key_4;
    std::string payload_data;
};

std::unique_ptr<std::vector<uint8_t>> create_message(uint8_t opcode, std::string& message) {
    std::unique_ptr<std::vector<uint8_t>> bytes(new std::vector<uint8_t>());
    uint8_t first_byte = 1 << 7 | opcode;
    bytes->push_back(first_byte);
    if (message.size() <= 125) {
        uint8_t second_byte = message.size();
        second_byte |= 1 << 7;
        bytes->push_back(second_byte);
    } else if (message.size() < 2 << 16) {
        uint8_t second_byte = 126;
        second_byte |= 1 << 7;
        bytes->push_back(second_byte);
        uint16_t message_size = (uint16_t) message.size();
        bytes->push_back((uint8_t) (message_size >> 8));
        bytes->push_back((uint8_t) message_size);
    } else {
        uint8_t second_byte = 127;
        second_byte |= 1 << 7;
        bytes->push_back(second_byte);
        uint64_t message_size = (uint64_t) message.size();
        bytes->push_back((uint8_t) (message_size >> 56));
        bytes->push_back((uint8_t) (message_size >> 48));
        bytes->push_back((uint8_t) (message_size >> 40));
        bytes->push_back((uint8_t) (message_size >> 32));
        bytes->push_back((uint8_t) (message_size >> 24));
        bytes->push_back((uint8_t) (message_size >> 16));
        bytes->push_back((uint8_t) (message_size >> 8));
        bytes->push_back((uint8_t) message_size);
    }

    uint8_t m1 = (uint8_t) (rand()%16);
    uint8_t m2 = (uint8_t) (rand()%16);
    uint8_t m3 = (uint8_t) (rand()%16);
    uint8_t m4 = (uint8_t) (rand()%16);
    bytes->push_back(m1);
    bytes->push_back(m2);
    bytes->push_back(m3);
    bytes->push_back(m4);

    uint8_t mask_keys[] {m1, m2, m3, m4};
    for (size_t i = 0; i < message.size(); i++) {
        bytes->push_back(message[i] ^ mask_keys[i%4]);
    }
    
    return bytes;
}

std::unique_ptr<std::vector<uint8_t>> create_text_message() {
    std::stringstream ss;
    std::string message_fragment = "0123456789";
    for (int i = 0; i < 60000; i++) {
        ss << message_fragment;
    }
    std::string message = ss.str();
    return create_message(0x1, message);
}

std::unique_ptr<std::vector<uint8_t>> create_binary_message() {
    std::stringstream ss;
    std::string message_fragment = "0123456789";
    for (int i = 0; i < 1000; i++) {
        ss << message_fragment;
    }
    std::string message = ss.str();
    return create_message(0x2, message);
}

std::unique_ptr<std::vector<uint8_t>> create_continuation_message() {
    std::stringstream ss;
    std::string message_fragment = "continuation";
    for (int i = 0; i < 60000; i++) {
        ss << message_fragment;
    }
    std::string message = ss.str();
    return create_message(0x0, message);
}

std::unique_ptr<std::vector<uint8_t>> create_ping_message() {
    std::string message("ping");
    return create_message(0x9, message);
}

std::unique_ptr<std::vector<uint8_t>> create_pong_message() {
    std::string message("pong");
    return create_message(0xa, message);
}

std::unique_ptr<std::vector<uint8_t>> create_close_message() {
    std::string message("1000");
    return create_message(0x8, message);
}

std::unique_ptr<Frame> parse(std::vector<uint8_t>& bytes) {
    std::unique_ptr<Frame> f(new Frame());
    f->fin = (bytes[0] >> 7) & 1;
    f->rsv1 = (bytes[0] >> 6) & 1;
    f->rsv2 = (bytes[0] >> 5) & 1;
    f->rsv3 = (bytes[0] >> 4) & 1;
    f->opcode = bytes[0] & 0xf;
    f->mask = (bytes[1] >> 7) & 1;
    uint8_t payload_len = bytes[1] & 0x7f; // 0x7f is b01111111
    int data_start_byte = 2;
    if (payload_len <= 125) {
        f->payload_len = payload_len;
    } else if (payload_len == 126) {
        f->payload_len = bytes[2] << 8 | bytes[3];
        data_start_byte += 2;
    } else {
        f->payload_len = (uint64_t)bytes[2] << 56 | (uint64_t)bytes[3] << 48 |
          (uint64_t)bytes[4] << 40 | (uint64_t)bytes[5] << 32 | (uint64_t)bytes[6] << 24 |
          (uint64_t)bytes[7] << 16 | (uint64_t)bytes[8] << 8 | (uint64_t)bytes[9];
        data_start_byte += 8;
    }

    uint8_t mask_keys[4] = {};
    if (f->mask) {
        mask_keys[0] = bytes[data_start_byte++];
        mask_keys[1] = bytes[data_start_byte++];
        mask_keys[2] = bytes[data_start_byte++];
        mask_keys[3] = bytes[data_start_byte++];

        f->mask_key_1 = mask_keys[0];
        f->mask_key_2 = mask_keys[1];
        f->mask_key_3 = mask_keys[2];
        f->mask_key_4 = mask_keys[3];
    }

    std::string message(bytes.begin() + data_start_byte, bytes.begin() + data_start_byte + f->payload_len);
    for (size_t i = 0; i < message.size(); i++) {
        message[i] = message[i] ^ mask_keys[i%4];
    }
    f->payload_data = message;
    return f;
};

void print_header(int size, std::vector<uint8_t>& bytes) {
    int i = 0;
    std::cout << "header: ";
    for (auto b : bytes) {
        if (i >= size) {
            break;
        }
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
        i++;
    }
    std::cout << std::endl;
}

void print_frame(std::unique_ptr<Frame>& f) {
    std::cout << "fin: " << std::boolalpha << f->fin << std::endl;
    std::cout << "rsv1: " << f->rsv1 << std::endl;
    std::cout << "rsv2: " << f->rsv2 << std::endl;
    std::cout << "rsv3: " << f->rsv3 << std::endl;
    std::cout << "opcode: " << "0x" << std::hex << static_cast<int>(f->opcode) << std::endl;
    std::cout << "payload_len: " << std::to_string(f->payload_len) << std::endl;
    std::cout << "masking_key: " << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(f->mask_key_1) << " "
        << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(f->mask_key_2) << " "
        << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(f->mask_key_3) << " "
        << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(f->mask_key_4) << " "
        << std::endl;
    std::cout << "first bytes of data: " << f->payload_data.substr(0, std::min(20, (int)f->payload_data.size())) << std::endl;
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "create and parse text message" << std::endl;
    std::unique_ptr<std::vector<uint8_t>> text_bytes = create_text_message();
    print_header(14, *text_bytes);
    std::unique_ptr<Frame> text_frame = parse(*text_bytes);
    print_frame(text_frame);
    std::cout << std::endl;

    std::cout << "create and parse binary message" << std::endl;
    std::unique_ptr<std::vector<uint8_t>> binary_bytes = create_binary_message();
    print_header(14, *binary_bytes);
    std::unique_ptr<Frame> binary_frame = parse(*binary_bytes);
    print_frame(binary_frame);
    std::cout << std::endl;

    std::cout << "create and parse continuation message" << std::endl;
    std::unique_ptr<std::vector<uint8_t>> cont_bytes = create_continuation_message();
    print_header(14, *cont_bytes);
    std::unique_ptr<Frame> cont_frame = parse(*cont_bytes);
    print_frame(cont_frame);
    std::cout << std::endl;

    std::cout << "create and parse ping message" << std::endl;
    std::unique_ptr<std::vector<uint8_t>> ping_bytes = create_ping_message();
    print_header(6, *ping_bytes);
    std::unique_ptr<Frame> ping_frame = parse(*ping_bytes);
    print_frame(ping_frame);
    std::cout << std::endl;

    std::cout << "create and parse pong message" << std::endl;
    std::unique_ptr<std::vector<uint8_t>> pong_bytes = create_pong_message();
    print_header(6, *pong_bytes);
    std::unique_ptr<Frame> pong_frame = parse(*pong_bytes);
    print_frame(pong_frame);
    std::cout << std::endl;

    std::cout << "create and parse close message" << std::endl;
    std::unique_ptr<std::vector<uint8_t>> close_bytes = create_close_message();
    print_header(6, *close_bytes);
    std::unique_ptr<Frame> close_frame = parse(*close_bytes);
    print_frame(close_frame);
    std::cout << std::endl;
}