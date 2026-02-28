#include "qns_player.hpp"
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void QNSPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_sync_x", "x"), &QNSPlayer::set_sync_x);
    ClassDB::bind_method(D_METHOD("get_sync_x"), &QNSPlayer::get_sync_x);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sync_x"), "set_sync_x", "get_sync_x");

    ClassDB::bind_method(D_METHOD("set_sync_y", "y"), &QNSPlayer::set_sync_y);
    ClassDB::bind_method(D_METHOD("get_sync_y"), &QNSPlayer::get_sync_y);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sync_y"), "set_sync_y", "get_sync_y");

    ClassDB::bind_method(D_METHOD("request_move", "x", "y"), &QNSPlayer::request_move);
    ClassDB::bind_method(D_METHOD("multicast_play_shoot_fx"), &QNSPlayer::multicast_play_shoot_fx);
}

void QNSPlayer::_ready() {
    QNSNetworkedNode::_ready(); 
    body = Object::cast_to<CharacterBody2D>(get_parent());
}

void QNSPlayer::_physics_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint() || !body) return;

    // Use the actual authority check from our base class
    if (has_authority()) {
        Input* input = Input::get_singleton();
        Vector2 input_vec = input->get_vector("ui_left", "ui_right", "ui_up", "ui_down");

        if (input_vec.length() > 0) {
            // Send the raw floats to the C++ server!
            request_move(input_vec.x, input_vec.y);
        }

        if (input->is_action_just_pressed("shoot")) {
            // rpc(qns_core::ct_hash("request_shoot"));
        }
    } 
    else {
        // Interpolate the visual body to the replicated sync floats
        Vector2 target_pos(sync_x, sync_y);
        body->set_global_position(body->get_global_position().lerp(target_pos, 10.0 * delta));
    }
}

void QNSPlayer::request_move(float x, float y) {
    // Safely pack pure C++ primitives over the network boundary
    rpc(qns_core::ct_hash("request_move"), x, y);
}

void QNSPlayer::multicast_play_shoot_fx() {
    // Spawn muzzle flash visually
}