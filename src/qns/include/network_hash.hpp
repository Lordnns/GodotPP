#pragma once
#include <cstdint>
namespace qns_core {
    inline constexpr uint32_t ct_hash(const char* str) {
        uint32_t hash = 2166136261u;
        while (*str) {
            hash ^= (uint8_t)(*str++);
            hash *= 16777619u;
        }
        return hash;
    }
}