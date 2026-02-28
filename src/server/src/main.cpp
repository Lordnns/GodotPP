#include <iostream>
#include <thread>
#include <chrono>
#include "quic_network_system.hpp"
#include "qns_serializer.hpp"
#include "network_hash.hpp"

using namespace qns_core;

// 1. The Pure C++ representation of a Player
struct ServerPlayer {
    uint32_t net_id;
    float position_x = 0.0f;
    float position_y = 0.0f;
    float speed = 200.0f;
};

// 2. The Server Application
class GameServer : public QuicNetworkSystemCore {
public:
    // This is triggered whenever the Rust core passes us an RPC from a client
    void handle_incoming_packet(uint32_t net_id, const uint8_t* data, size_t len) override {
        QNSDeserializer d(data, len);
        uint8_t packet_type = d.read<uint8_t>();

        std::cout << "Connection Received" << std::endl;
        
        if (packet_type == 1) { // TYPE 1: RPC
            uint32_t target_net_id = d.read<uint32_t>();
            uint32_t rpc_hash = d.read<uint32_t>();
            uint8_t arg_count = d.read<uint8_t>();

            // Find the pure C++ player struct
            void* raw_ptr = linking_context->get_entity(target_net_id);
            if (!raw_ptr) return;
            ServerPlayer* player = static_cast<ServerPlayer*>(raw_ptr);

            // Execute Authoritative Server Logic
            if (rpc_hash == ct_hash("request_move")) {
                // We know this RPC sends two floats. Read them directly.
                float input_x = d.read<float>();
                float input_y = d.read<float>();

                // Basic Server-Side Physics / Movement Application
                player->position_x += input_x * player->speed * 0.016f; // Assuming 60Hz tick (1/60 = 0.016)
                player->position_y += input_y * player->speed * 0.016f;
            }
        }
    }

    // Checks for new QUIC sessions and spawns players for them
    void process_new_connections() {
        for (const auto& [id, session] : session_manager->get_all_sessions()) {
            // If a session exists, but no entity is linked to it, they are new!
            if (!linking_context->get_entity(id)) {
                
                // 1. Create the server-side memory
                ServerPlayer* new_player = new ServerPlayer{id, 0.0f, 0.0f};
                linking_context->register_entity(id, new_player);
                std::cout << "Player " << id << " joined. Spawning..." << std::endl;

                // 2. Tell everyone else to spawn this new player
                QNSSerializer spawn_packet;
                spawn_packet.write<uint8_t>(2); // TYPE 2: SPAWN
                spawn_packet.write<uint32_t>(id); // Net ID
                spawn_packet.write<uint32_t>(id); // Owner ID
                spawn_packet.write_string("res://prefabs/player.tscn"); // Godot scene to load
                broadcast_to_layer_4(spawn_packet.get_buffer(), true);

                // 3. Tell the NEW player to spawn all the EXISTING players
                for (const auto& [existing_id, existing_ptr] : linking_context->get_all_entities()) {
                    if (existing_id != id) {
                        QNSSerializer existing_spawn;
                        existing_spawn.write<uint8_t>(2);
                        existing_spawn.write<uint32_t>(existing_id);
                        existing_spawn.write<uint32_t>(existing_id);
                        existing_spawn.write_string("res://prefabs/player.tscn");
                        route_to_layer_4(id, existing_spawn.get_buffer(), true);
                    }
                }
            }
        }
    }

    // Sends the exact coordinates of everyone to everyone
    void broadcast_snapshots() {
        for (const auto& [net_id, ptr] : linking_context->get_all_entities()) {
            ServerPlayer* player = static_cast<ServerPlayer*>(ptr);

            QNSSerializer s;
            s.write<uint8_t>(0); // TYPE 0: SNAPSHOT
            s.write<uint32_t>(player->net_id);
            
            // We are sending 2 properties (X and Y)
            s.write<uint8_t>(2); 
            
            s.write<uint32_t>(ct_hash("position_x"));
            s.write<float>(player->position_x);
            
            s.write<uint32_t>(ct_hash("position_y"));
            s.write<float>(player->position_y);

            // Broadcast unreliably (UDP standard for positional data)
            broadcast_to_layer_4(s.get_buffer(), false); 
        }
    }
};

// --- PURE C++ ENTRY POINT ---
int main() {
    GameServer server;
    server.listen(7777);
    std::cout << "QNS Authoritative Server started on port 7777" << std::endl;

    // 60 Hz Server Tick Rate
    const auto TICK_RATE = std::chrono::milliseconds(16); 

    while (true) {
        auto tick_start = std::chrono::steady_clock::now();

        // 1. Receive data from network
        server.poll_network_events();
        
        // 2. Spawn new players
        server.process_new_connections();
        
        // 3. Send out the state of the world
        server.broadcast_snapshots();

        // 4. Sleep to maintain 60 FPS
        auto tick_end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(TICK_RATE - (tick_end - tick_start));
    }

    return 0;
}