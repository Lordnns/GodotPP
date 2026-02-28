#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <godot_cpp/classes/character_body2d.hpp>

namespace godot {
    class Player : public CharacterBody2D {
        GDCLASS(Player, CharacterBody2D)

    protected:
        static void _bind_methods();

    public:
        Player();
        ~Player();
        void _physics_process(double delta) override;
    };
}
#endif