#pragma once

#ifdef QNS_WITH_GODOT

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "qns_replication_builder.hpp"
#include "qns_serializer.hpp"
#include "quic_network_system.hpp"
#include "qns_godot_utils.hpp"

namespace godot {
    class QNSNetworkedNode : public Node {
        GDCLASS(QNSNetworkedNode, Node)
    private:
        qns_core::QNSReplicationBuilder rep_config;
        uint32_t my_net_id = 0;
        uint32_t owner_session_id = 0;
        QuicNetworkSystem* qns_system = nullptr; 

    protected:
        static void _bind_methods() {}
        virtual void _setup_replication(qns_core::QNSReplicationBuilder& builder) {}

    public:
        void _ready() override {
            _setup_replication(rep_config);
            Object* singleton = Engine::get_singleton()->get_singleton("QuicNetworkSystem");
            qns_system = Object::cast_to<QuicNetworkSystem>(singleton);
        }
        
        void set_network_id(uint32_t id) { my_net_id = id; }
        uint32_t get_network_id() const { return my_net_id; }
        
        void set_network_owner(uint32_t id) { owner_session_id = id; }
        uint32_t get_network_owner() const { return owner_session_id; }
        
        bool has_authority() const {
            if (!qns_system) return false;
            return qns_system->is_server(); 
        }
        
        const qns_core::QNSReplicationBuilder& get_config() const { return rep_config; }

        template<typename... Args>
        void rpc(uint32_t rpc_hash, Args... args) {
            if (!qns_system || rep_config.rpcs.find(rpc_hash) == rep_config.rpcs.end()) return;
            const RPCDefinition& def = rep_config.rpcs[rpc_hash];

            qns_core::QNSSerializer s;
            s.write((uint8_t)1); // Packet Type 1 = RPC
            s.write(my_net_id);
            s.write(rpc_hash);
            s.write((uint8_t)sizeof...(args));
            (s.write_variant(Variant(args)), ...); 

            // Bridge to Godot Variant system via Utility
            (QNSGodotUtils::write_variant(s, Variant(args)), ...); 

            // Convert core buffer to vector for agnostic routing
            qns_system->route_to_layer_4(0, s.get_buffer(), def.is_reliable);
        }
    };
}
#endif