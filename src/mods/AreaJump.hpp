#pragma once

#include "../mod.hpp"

struct Room;

class AreaJump : public Mod {
public:
    AreaJump() = default;

    std::string get_mod_name() override { return "AreaJump"; };

    struct Room {
      int id;
      const char* name;
    };

    static std::array<Room, 83> room_items;

    // BP:
    static bool randomize_bp_toggle;
    static bool bp_boss_rush_toggle;
    static uintptr_t bp_jump_hook1_continue;
    static uintptr_t bp_jump_hook2_continue;
    static uintptr_t bp_jump_hook3_continue;
    void toggle_randomized_bp(bool enable);
    static const Room* bp_stage(int floor);
    static void jump_to_stage(const Room* stage);
    static bool is_valid_room_id(int id);
    static const Room* find_room_by_id(int id);
    static const Room* find_room_by_name(const csys::String& name);
    std::optional<std::string> on_initialize() override;

    void on_gui_frame(int display) override;
    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:

    // BP:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
