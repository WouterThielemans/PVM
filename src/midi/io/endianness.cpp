#include "endianness.h"
#include <algorithm>

void io::switch_endianness(uint16_t* n){
    auto bytes = reinterpret_cast<uint8_t*>(n);
    std::reverse(bytes, bytes + 2);
}

void io::switch_endianness(uint32_t* n){
    auto bytes = reinterpret_cast<uint8_t*>(n);
    std::reverse(bytes, bytes + 4);
}

void io::switch_endianness(uint64_t* n){
    auto bytes = reinterpret_cast<uint8_t*>(n);
    std::reverse(bytes, bytes + 8);
}