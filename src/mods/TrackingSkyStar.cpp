#include "TrackingSkyStar.hpp"

#if 1
bool TrackingSkyStar::mod_enabled{false};
uintptr_t TrackingSkyStar::tracking_sky_star_continue{NULL};
static float turn_speed{1.0f};
// static float default_turn_speed{0.42f};

naked void tracking_sky_star_proc(void) {
    _asm {
		cmp byte ptr [TrackingSkyStar::mod_enabled], 0
        je originalcode

        cmp dword ptr [esi+0x0000141C], 00 // how far is stick held
        ja originalcode
        cmp dword ptr [esi+0x000016D0], 00 // locked off?
        je originalcode
        pushad
        mov ebx, 1
        mov ecx, 4
        // mov edx,3 // unused
        fld dword ptr [turn_speed]
        mov edx, [esi]
        mov eax, [edx+0x000002B4]
        push 00
        push ecx
        fstp dword ptr [esp]
        push 00
        push 00
        push 00
        push ebx
        mov ecx, esi
        call eax
        popad

		originalcode:
        fld dword ptr [esi+0x0000170C]
		jmp dword ptr [TrackingSkyStar::tracking_sky_star_continue]
    }
}

std::optional<std::string> TrackingSkyStar::on_initialize() {
    if (!install_hook_offset(0x3CA81A, hook, &tracking_sky_star_proc, &TrackingSkyStar::tracking_sky_star_continue, 6)) {
        spdlog::error("Failed to init TrackingSkyStar mod\n");
        return "Failed to init TrackingSkyStar mod";
    }

    return Mod::on_initialize();
}

void TrackingSkyStar::on_gui_frame() {
    ImGui::Checkbox("Auto Facing Sky Star", &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Sky star will auto face the enemy you're locked on to"));
}

void TrackingSkyStar::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("tracking_sky_star").value_or(false);
}

void TrackingSkyStar::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("tracking_sky_star", mod_enabled);
}

#endif
