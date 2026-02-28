#pragma once
#include <cstdint>
#include <array>

namespace qns_core {

    enum class SessionState {
        CONNECTING,
        AUTHENTICATED,
        ACTIVE,
        DISCONNECTING
    };

    struct QNSSession {
        uint32_t net_id;                    // Internal Game ID (e.g., 1, 2, 3...)
        std::array<uint8_t, 16> rust_uuid;  // Raw UUID from Layer 4 (Rust)
        SessionState state;
        uint32_t last_seen_tick;
        float rtt;                          // Round Trip Time (Ping)
    };

}