#include "FastRoundTrip.hpp"

#if 1
bool FastRoundTrip::mod_enabled = false;

uintptr_t FastRoundTrip::jmp_ret{ NULL };
float FastRoundTrip::start_frame = 80.0f;

naked void detour1(void) { // player in eax
    _asm {
		cmp byte ptr [FastRoundTrip::mod_enabled], 0
        je originalcode
        jmp newcode
    newcode:
        fld dword ptr [FastRoundTrip::start_frame]
        jmp contcode
    originalcode:
        fldz
    contcode:
        mov al, bl
        fstp dword ptr [esp+04]
		jmp dword ptr [FastRoundTrip::jmp_ret]
    }
}

std::optional<std::string> FastRoundTrip::on_initialize() {
    if (!install_hook_offset(0x3CF299, hook, &detour1, &FastRoundTrip::jmp_ret, 8)) {
        spdlog::error("Failed to init FastRoundTrip mod 1\n");
        return "Failed to init FastRoundTrip mod 1";
    }
    return Mod::on_initialize();
}

void FastRoundTrip::on_gui_frame(int display) {
    if (display == 2) {
        ImGui::Checkbox(_("Fast Round Trip"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Round Trip throw animation starts at frame 80"));
    }
}

void FastRoundTrip::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("fast_round_trip").value_or(false);
}

void FastRoundTrip::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_round_trip", mod_enabled);
}

#endif
