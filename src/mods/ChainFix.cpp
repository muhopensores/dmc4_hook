#include "ChainFix.hpp"

// Addresses: DX9 exe, RE note Vibed\RE\chain_cnschain.md 3.1 / 3.2.
// sDevil4Main::mpInstance 0xE5574C: +0x68 mDeltaTime (60 fps frames, x mGlobalSpeed); sWorkRate::mpInstance 0xE558D0: +0x20
// the game speed (what the other chains scale by).
static constexpr uintptr_t kBlendFade  = 0x42DC30;  // cCnsChain::move: the blend-fade step, 1.0 for fixed-step chains (to 0x42DC46)
static constexpr uintptr_t kSolverOne  = 0x430A72;  // cCnsChain::calcJointImpl: the 1.0 a fixed-step chain solves with (movss xmm0, [B9A23C])
static constexpr uintptr_t kDamping    = 0x431B80;  // calcJointImpl: movss xmm0, [edi+53Ch] (mDamping) before vel *= it (to 0x431B88)
static constexpr uintptr_t kTurbSkip   = 0x42DDBD;  // move: jmp past the turbulence add after a re-roll (eb 54)

bool ChainFix::mod_enabled = true;
bool ChainFix::follow_slow_motion = false;
bool ChainFix::turbulence = false;
float ChainFix::step = 1.0f;
uintptr_t ChainFix::jmp_ret_blend = NULL;
uintptr_t ChainFix::jmp_ret_damp = NULL;

static float c_one = 1.0f, c_two = 2.0f, c_zero = 0.0f;

// Replaces 0x42DC30-0x42DC46: xmm4 = the blend-fade step (the chain's time scale, or for a fixed-step chain the step this
// frame, which the solver then reads through the patched load), xmm1 = 1.0 (the turbulence timer's threshold further on).
naked void detour_blend() {
    _asm {
        push eax
        movss xmm4, [ebx+0x14]
        cmp byte ptr [ebx+0x18], 0
        je done
        movss xmm4, [c_one]
        cmp byte ptr [ChainFix::mod_enabled], 0
        je store
        mov eax, ds:[0xE5574C]          // sDevil4Main::mpInstance
        movss xmm4, [eax+0x68]
        cmp byte ptr [ChainFix::follow_slow_motion], 0
        je clamp
        mov eax, ds:[0xE558D0]          // sWorkRate::mpInstance
        mulss xmm4, [eax+0x20]
    clamp:
        minss xmm4, [c_two]             // the game's own clamp for other chains
        maxss xmm4, [c_zero]
    store:
        movss [ChainFix::step], xmm4
    done:
        movss xmm1, [c_one]
        pop eax
        jmp dword ptr [ChainFix::jmp_ret_blend]
    }
}

// xmm0 = mDamping ^ t (t = the solve's step, [esp+0x1C]; only reached with t > 0). x87 only and no globals written: the
// solver may run on job threads.
naked void detour_damp() {
    _asm {
        movss xmm0, [edi+0x53C]
        cmp byte ptr [ChainFix::mod_enabled], 0
        je back
        comiss xmm0, [c_zero]
        jbe back                        // pow of 0 / a negative: keep the game's value
        sub esp, 4
        fld dword ptr [esp+4+0x1C]      // t
        fld dword ptr [edi+0x53C]       // d
        fyl2x                           // t log2 d
        fld st(0)
        frndint                         // n
        fsub st(1), st(0)               // f = x - n, |f| <= 0.5
        fxch st(1)
        f2xm1
        fld1
        faddp st(1), st(0)              // 2^f
        fscale                          // 2^f 2^n
        fstp st(1)
        fstp dword ptr [esp]
        movss xmm0, [esp]
        add esp, 4
    back:
        jmp dword ptr [ChainFix::jmp_ret_damp]
    }
}

std::optional<std::string> ChainFix::on_initialize() {
    if (!install_hook_absolute(kBlendFade, hook_blend, &detour_blend, &jmp_ret_blend, 0x16)) {
        spdlog::error("Failed to init ChainFix mod (blend)\n");
        return "Failed to init ChainFix mod";
    }
    if (!install_hook_absolute(kDamping, hook_damp, &detour_damp, &jmp_ret_damp, 8)) {
        spdlog::error("Failed to init ChainFix mod (damping)\n");
        return "Failed to init ChainFix mod";
    }
    // the solver's 1.0 for fixed-step chains now loads `step` (1.0 while the mod is off: detour_blend stores 1.0 then)
    const auto a = reinterpret_cast<uintptr_t>(&step);
    install_patch_absolute(kSolverOne, patch_step, reinterpret_cast<const char*>(&a), 4);
    return Mod::on_initialize();
}

void ChainFix::toggle_turbulence(bool enable) {
    if (enable)
        install_patch_absolute(kTurbSkip, patch_turbulence, "\xEB\x00", 2); // fall into the add after a re-roll
    else
        patch_turbulence.reset();
}

void ChainFix::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A)
        return;
    ImGui::Checkbox(_("Chain Physics at Any Frame Rate"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Coats, hair and other model chains move the same at any frame rate.\n"
                  "The game steps the players' chains once per drawn frame, so with the frame rate unlocked the coat "
                  "swings faster. Nothing changes at 60 fps."));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Player Chains Follow Slow Motion"), &follow_slow_motion);
        ImGui::SameLine();
        help_marker(_("The players' chains slow down with the game speed, as enemies' chains do. The game keeps them at "
                      "full speed."));
        ImGui::Unindent(lineIndent);
    }
    if (ImGui::Checkbox(_("Chain Turbulence"), &turbulence))
        toggle_turbulence(turbulence);
    ImGui::SameLine();
    help_marker(_("Turn on the random wind (turbulence) some chains have. A bug keeps it off at 60 fps in the game, so "
                  "this changes how those chains look."));
}

void ChainFix::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("chain_fix").value_or(true);
    follow_slow_motion = cfg.get<bool>("chain_fix_slow_motion").value_or(false);
    turbulence = cfg.get<bool>("chain_fix_turbulence").value_or(false);
    toggle_turbulence(turbulence);
}

void ChainFix::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("chain_fix", mod_enabled);
    cfg.set<bool>("chain_fix_slow_motion", follow_slow_motion);
    cfg.set<bool>("chain_fix_turbulence", turbulence);
}
