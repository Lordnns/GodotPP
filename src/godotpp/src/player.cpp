#include "player.hpp"
#include <godot_cpp/classes/input.hpp>

using namespace godot;

void Player::_bind_methods() {}

Player::Player() {}

Player::~Player() {}

void Player::_physics_process(double delta) {
    Vector2 input_dir = Input::get_singleton()->get_vector("ui_left", "ui_right", "ui_up", "ui_down");
    set_velocity(input_dir * 300.0);
    move_and_slide();
}