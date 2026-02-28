#pragma once
#include <map>
#include <vector>
#include "qns_session.hpp"
#include "linking_context.hpp"

namespace qns_core {

    class QNSSessionManager {
    private:
        uint32_t next_net_id = 1;
        std::map<uint32_t, QNSSession> active_sessions;
        std::map<std::array<uint8_t, 16>, uint32_t> uuid_to_net_id;
        LinkingContext* linking_context;

    public:
        QNSSessionManager(LinkingContext* lc) : linking_context(lc) {}

        // Called when Layer 4 detects a new QUIC connection
        uint32_t create_session(const std::array<uint8_t, 16>& uuid) {
            uint32_t id = next_net_id++;
            QNSSession session;
            session.net_id = id;
            session.rust_uuid = uuid;
            session.state = SessionState::CONNECTING;
        
            active_sessions[id] = session;
            uuid_to_net_id[uuid] = id;
            return id;
        }

        // Lookup session by Rust UUID
        QNSSession* get_session_by_uuid(const std::array<uint8_t, 16>& uuid) {
            if (uuid_to_net_id.count(uuid)) {
                return &active_sessions[uuid_to_net_id[uuid]];
            }
            return nullptr;
        }
        
        QNSSession* get_session(uint32_t id) {
            return active_sessions.count(id) ? &active_sessions[id] : nullptr;
        }

        void terminate_session(uint32_t net_id) {
            if (active_sessions.count(net_id)) {
                uuid_to_net_id.erase(active_sessions[net_id].rust_uuid);
                active_sessions.erase(net_id);
                // Clean up the Linking Context (Layer 6)
                linking_context->unregister_entity(net_id);
            }
        }

        const std::map<uint32_t, QNSSession>& get_all_sessions() const { return active_sessions; }
    };

}