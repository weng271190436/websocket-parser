#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <stdlib.h>

struct Frame {
    bool fin;
    bool rsv1;
    bool rsv2;
    bool rsv3;
    uint16_t opcode;
    bool mask;
    uint64_t payload_len;
    uint32_t masking_key;
    std::string payload_data;
};

std::vector<uint8_t>* create_message() {
    std::stringstream ss;
    std::string message_fragment = "0123456789";
    for (int i = 0; i < 60000; i++) {
        ss << message_fragment;
    }
    std::string message = ss.str();
    // fin bit and text frame opcode
    std::vector<uint8_t>* bytes = new std::vector<uint8_t>();
    uint8_t first_byte = 1 << 7 | 1;
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

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    bytes->push_back((uint8_t) (rand()%16));
    bytes->push_back((uint8_t) (rand()%16));
    bytes->push_back((uint8_t) (rand()%16));
    bytes->push_back((uint8_t) (rand()%16));
    return bytes;
}

Frame* parse(std::vector<uint8_t>& bytes) {
    Frame* f = new Frame();
    return f;
};

int main() {
    std::vector<uint8_t>* bytes = create_message();
    for (auto b : *bytes) {
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
    }
    std::cout << std::endl;
    Frame* f = parse(*bytes);
    delete f;
    delete bytes;
}