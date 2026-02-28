#pragma once
#include "../qns/qns_networked_node.hpp"
#include "../qns/qns_macros.hpp"
#include <godot_cpp/classes/character_body2d.hpp>

namespace godot {

    class QNSPlayer : public QNSNetworkedNode {
        GDCLASS(QNSPlayer, QNSNetworkedNode)

    private:
        float health = 100.0f;
        
        // BREAKDOWN: Use raw floats for the network boundary
        float sync_x = 0.0f;
        float sync_y = 0.0f;

        CharacterBody2D* body = nullptr;

    protected:
        static void _bind_methods();

        void _setup_replication(qns_core::QNSReplicationBuilder& builder) override {
            // Match the pure C++ server properties exactly!
            builder.add_property(qns_core::ct_hash("position_x"), "sync_x");
            builder.add_property(qns_core::ct_hash("position_y"), "sync_y");
            builder.add_property(qns_core::ct_hash("health"), "health"); // Note: Requires on_rep setup manually if not using your macro

            // RPCs
            builder.add_rpc(qns_core::ct_hash("request_move"), "request_move", qns_core::RPCMode::SERVER, false);
            builder.add_rpc(qns_core::ct_hash("multicast_play_shoot_fx"), "multicast_play_shoot_fx", qns_core::RPCMode::MULTICAST, true);
        }

    public:
        void _ready() override;
        void _physics_process(double delta) override;

        // Setters for the Deserializer
        void set_sync_x(float x) { sync_x = x; }
        float get_sync_x() const { return sync_x; }
        void set_sync_y(float y) { sync_y = y; }
        float get_sync_y() const { return sync_y; }
        void set_health(float h) { health = h; }
        float get_health() const { return health; }

        // Local RPC Wrappers
        void request_move(float x, float y);
        void multicast_play_shoot_fx();
    };
}