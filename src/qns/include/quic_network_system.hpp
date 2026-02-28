#ifndef QUIC_NETWORK_SYSTEM_HPP
#define QUIC_NETWORK_SYSTEM_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "qns_session_manager.hpp"
#include "linking_context.hpp"

namespace qns_core {

    class QuicNetworkSystemCore {
    protected:
        void* rust_peer = nullptr; 
        bool is_server_mode = false;
        uint32_t next_allocated_net_id = 100;
        
        QNSSessionManager* session_manager = nullptr;
        LinkingContext* linking_context = nullptr;

    public:
        QuicNetworkSystemCore();
        virtual ~QuicNetworkSystemCore();

        void listen(int port);
        void connect_to_server(const std::string& ip, int port);
        void disconnect_from_network();
        
        bool is_server() const { return is_server_mode; }
        
        void route_to_layer_4(uint32_t target_net_id, const std::vector<uint8_t>& data, bool reliable);
        void broadcast_to_layer_4(const std::vector<uint8_t>& data, bool reliable);
        
        virtual void handle_incoming_packet(uint32_t net_id, const uint8_t* data, size_t len) = 0;
        void poll_network_events();
    };
}

// --- GODOT WRAPPER ---
#ifdef QNS_WITH_GODOT
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {
    class QuicNetworkSystem : public Node, public qns_core::QuicNetworkSystemCore {
        GDCLASS(QuicNetworkSystem, Node)
    protected:
        static void _bind_methods();
    public:
        void handle_incoming_packet(uint32_t net_id, const uint8_t* data, size_t len) override;
        
        void _process(double delta) override { poll_network_events(); }
        void connect_to_server_godot(String ip, int port);
		void listen_godot(int port);
        void disconnect_godot();
        bool is_server_godot() const;
    };
}
#endif

#endif