#pragma once

#include "../mod.hpp"

// Fixes to the game's own model chains (cCnsChain, RE note Vibed\RE\chain_cnschain.md):
//   - player chains take one full step per rendered frame whatever the frame rate (their fixed-step byte +0x18): with the
//     frame rate unlocked the coat swings fast. They now step by the frame's real length (sDevil4Main mDeltaTime, 1.0
//     at the 60 fps lock, so nothing changes there), optionally slowed with the game's speed (slow motion).
//   - damping is applied once per solve, not per unit of time: now mDamping ^ step, for every chain (the same at a full
//     step).
//   - turbulence (mTurbulence) is re-rolled whenever its timer reaches one step and never added on that frame, so at a
//     steady 60 fps it never applies. Optional (36 vanilla chains have a value nobody has seen in the game).
class ChainFix : public Mod {
public:
    ChainFix() = default;
    std::string get_mod_name() override { return "ChainFix"; };
    std::vector<std::string> get_search_terms() override { return {"chain", "coat", "physics", "fps"}; }

    static bool mod_enabled;        // frame-rate step + damping
    static bool follow_slow_motion; // player chains slow down with the game speed (sWorkRate +0x20)
    static bool turbulence;         // mTurbulence applies every frame
    static float step;              // the step player chains take this frame (read by the solver's patched load)
    static uintptr_t jmp_ret_blend, jmp_ret_damp;

    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    void toggle_turbulence(bool enable);

    std::unique_ptr<FunctionHook> hook_blend, hook_damp;
    std::unique_ptr<Patch> patch_step, patch_turbulence;
};
