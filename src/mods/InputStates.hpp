#pragma once

#include "../mod.hpp"

class InputStates : public Mod {
public:
    InputStates() = default;

    static uintptr_t jmp_return;
    static uintptr_t jmp_return2;
    static uintptr_t jmp_return3;

    static uint32_t inputpressed;
    static float input_timer;
    static bool touchpad_rose_enabled;

    // Taunt Ecstasy's Rose, per player (player_info's index): the press, the buffered Rose, and the timer that hides
    // Lucifer afterwards. The hooks pass the player they run for.
    static void __stdcall rose_tick(uintptr_t player);    // each tick: the buffer, the hide timer
    static void __stdcall rose_press(uintptr_t player);   // Taunt pressed in the air
    // the Rose's hide timer runs for this player (his Lucifer stays out: ForceLucifer, the change to Lucifer)
    static bool __stdcall rose_active(uintptr_t player);

    std::string get_mod_name() override { return "InputStates"; };
    std::vector<std::string> get_search_terms() override {
        return {
            "taunt ecstasy"
        };
    }
    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
};
