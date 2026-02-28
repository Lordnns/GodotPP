#include "quic_network_system.hpp"
#include "qns_godot_utils.hpp"
#include "qns_networked_node.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

// FIXED: Using the custom CMake definition
#ifdef QNS_WITH_GODOT
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#endif

// --- Layer 4: Rust FFI Declarations ---
extern "C" {
    void* qns_create_peer();
    void qns_connect(void* peer_ptr, const char* ip, uint16_t port);
    void qns_listen(void* peer_ptr, uint16_t port);
    void qns_destroy_peer(void* peer_ptr);
    void* qns_poll_event(void* peer_ptr, uint8_t* out_type, uint8_t* out_uuid, uint16_t* out_stream_id, const uint8_t** out_data_ptr, size_t* out_data_len);
    void qns_free_bytes(void* bytes_ptr);
    void qns_send(void* peer_ptr, const uint8_t* uuid_bytes, uint16_t stream_index, bool is_reliable, const uint8_t* data_ptr, size_t data_len);
}

namespace qns_core {

QuicNetworkSystemCore::QuicNetworkSystemCore() {
    rust_peer = qns_create_peer();
    linking_context = new LinkingContext();
    session_manager = new QNSSessionManager(linking_context);
}

QuicNetworkSystemCore::~QuicNetworkSystemCore() {
    if (rust_peer) {
        qns_destroy_peer(rust_peer);
    }
    delete session_manager;
    delete linking_context;
}

void QuicNetworkSystemCore::listen(int port) {
    if (rust_peer) {
        is_server_mode = true;
        qns_listen(rust_peer, (uint16_t)port);
    }
}

void QuicNetworkSystemCore::connect_to_server(const std::string& ip, int port) {
    if (rust_peer) {
        is_server_mode = false;
        qns_connect(rust_peer, ip.c_str(), (uint16_t)port);
    }
}

void QuicNetworkSystemCore::disconnect_from_network() {
    if (rust_peer) {
        qns_destroy_peer(rust_peer);
        rust_peer = qns_create_peer();
    }
    is_server_mode = false;
}

void QuicNetworkSystemCore::poll_network_events() {
    if (!rust_peer) return;

    uint8_t event_type;
    uint8_t uuid_bytes[16];
    uint16_t stream_id;
    const uint8_t* raw_data = nullptr;
    size_t data_len = 0;
    void* bytes_handle = nullptr;

    while ((bytes_handle = qns_poll_event(rust_peer, &event_type, uuid_bytes, &stream_id, &raw_data, &data_len))) {
        
		std::cout << "Polling" << std::endl;
        std::array<uint8_t, 16> safe_uuid;
        std::copy(std::begin(uuid_bytes), std::end(uuid_bytes), safe_uuid.begin());

        if (event_type == 1) { // CONNECT
			std::cout << "[Server] Connect Event Received" << std::endl;
            session_manager->create_session(safe_uuid);
			std::cout << "[Server] New QUIC Session Created" << std::endl;
        } 
        else if (event_type == 2) { // DISCONNECT
            auto* s = session_manager->get_session_by_uuid(safe_uuid);
            if (s) session_manager->terminate_session(s->net_id);
        } 
        else if (event_type == 3) { // MESSAGE
            auto* s = session_manager->get_session_by_uuid(safe_uuid);
            if (s) this->handle_incoming_packet(s->net_id, raw_data, data_len);
        }
        qns_free_bytes(bytes_handle);
    }
}

void QuicNetworkSystemCore::route_to_layer_4(uint32_t target_net_id, const std::vector<uint8_t>& data, bool reliable) {
    auto* s = session_manager->get_session(target_net_id);
    if (rust_peer && s) {
        qns_send(rust_peer, s->rust_uuid.data(), (reliable ? 0 : 1), reliable, data.data(), data.size());
    }
}

void QuicNetworkSystemCore::broadcast_to_layer_4(const std::vector<uint8_t>& data, bool reliable) {
    if (!rust_peer) return;
    for (auto const& [id, session] : session_manager->get_all_sessions()) {
        qns_send(rust_peer, session.rust_uuid.data(), (reliable ? 0 : 1), reliable, data.data(), data.size());
    }
}

} // namespace qns_core

// --- Godot Wrapper Implementation ---
// FIXED: Using the custom CMake definition
#ifdef QNS_WITH_GODOT
namespace godot {

void QuicNetworkSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("listen", "port"), &QuicNetworkSystem::listen_godot);
    ClassDB::bind_method(D_METHOD("connect_to_server", "ip", "port"), &QuicNetworkSystem::connect_to_server_godot);
    ClassDB::bind_method(D_METHOD("disconnect"), &QuicNetworkSystem::disconnect_godot);
    ClassDB::bind_method(D_METHOD("is_server"), &QuicNetworkSystem::is_server_godot);
}

void QuicNetworkSystem::listen_godot(int port) {
    listen(port);
}

void QuicNetworkSystem::connect_to_server_godot(String ip, int port) {
    connect_to_server(ip.utf8().get_data(), port);
}

void QuicNetworkSystem::disconnect_godot() {
    disconnect_from_network();
}

bool QuicNetworkSystem::is_server_godot() const {
    return is_server();
}

void QuicNetworkSystem::handle_incoming_packet(uint32_t net_id, const uint8_t* data, size_t len) {
    qns_core::QNSDeserializer d(data, len);
    uint8_t packet_type = d.read<uint8_t>();

    void* raw_ptr = linking_context->get_entity(net_id);
    if (!raw_ptr) return;
    QNSNetworkedNode* entity = static_cast<QNSNetworkedNode*>(raw_ptr);

    if (packet_type == 1) { // RPC
        uint32_t target_net_id = d.read<uint32_t>();
        uint32_t rpc_hash = d.read<uint32_t>();
        uint8_t arg_count = d.read<uint8_t>();

        const auto& rpcs = entity->get_config().rpcs;
        if (rpcs.count(rpc_hash)) {
            const auto& def = rpcs.at(rpc_hash);
            Array godot_args;
            for (int i = 0; i < arg_count; ++i) {
                godot_args.push_back(QNSGodotUtils::read_variant(d)); 
            }
            entity->callv(def.name.c_str(), godot_args);
        }
    }
    else if (packet_type == 0) { // SNAPSHOT
        uint8_t prop_count = d.read<uint8_t>();
        const auto& props = entity->get_config().properties;

        for (int i = 0; i < prop_count; ++i) {
            uint32_t prop_hash = d.read<uint32_t>();
            Variant new_value = QNSGodotUtils::read_variant(d);

            if (props.count(prop_hash)) {
                const auto& prop = props.at(prop_hash);
                Variant old_value = entity->get(prop.name.c_str());
                if (old_value != new_value) {
                    entity->set(prop.name.c_str(), new_value);
                    if (prop.has_on_rep) entity->call(prop.on_rep_func.c_str(), old_value);
                }
            }
        }
    }
    else if (packet_type == 2) { // SPAWN
        uint32_t new_net_id = d.read<uint32_t>();
        uint32_t owner_id = d.read<uint32_t>();
        std::string prefab_path_raw = d.read_string();
        String prefab_path = String(prefab_path_raw.c_str());

        if (linking_context->get_entity(new_net_id)) return;

        Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(prefab_path);
        if (scene.is_valid()) {
            Node* instance = scene->instantiate();
            QNSNetworkedNode* net_node = Object::cast_to<QNSNetworkedNode>(instance);
            if (!net_node) {
                TypedArray<Node> children = instance->find_children("*", "QNSNetworkedNode");
                if (children.size() > 0) net_node = Object::cast_to<QNSNetworkedNode>(children[0]);
            }

            if (net_node) {
                net_node->set_network_id(new_net_id);
                net_node->set_network_owner(owner_id);
                linking_context->register_entity(new_net_id, static_cast<void*>(net_node));
            }
            get_tree()->get_current_scene()->add_child(instance);
        }
    }
}
} // namespace godot
#endif