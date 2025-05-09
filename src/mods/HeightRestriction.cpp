#include "HeightRestriction.hpp"

#if 1
bool HeightRestriction::toggle_nero = false;
bool HeightRestriction::toggle_dante = false;

uintptr_t HeightRestriction::air_hike_jmp_ret = NULL;
bool HeightRestriction::air_hike_nero = false;
bool HeightRestriction::air_hike_dante = false;

void HeightRestriction::ToggleNero(bool enable) {
    if (enable) {
        install_patch_offset(0x3E614B, patch_buster, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E5F8D, patch_split, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E6248, patch_calibur, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E62B6, patch_ex_calibur, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E60E4, patch_snatch, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E603F, patch_rave, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E5FE1, patch_double_down, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E4B12, patch_rev, "\x90\x90", 2);
    }
    else {
        patch_buster.reset();
        patch_split.reset();
        patch_calibur.reset();
        patch_ex_calibur.reset();
        patch_snatch.reset();
        patch_rave.reset();
        patch_double_down.reset();
        patch_rev.reset();
    }
}

void HeightRestriction::ToggleDante(bool enable) {
    if (enable) {
        install_patch_offset(0x3B764E, patch_dante, "\x0F\x84\x7B\x03\x00\x00", 6);
    }
    else {
        patch_dante.reset();
    }
}

uintptr_t air_hike_jmp_alt = 0x8038FE;
naked void detour_air_hike() { // player in esi
    _asm {
            cmp byte ptr [esi+0x1494], 1 // controller id nero
            je CheckNero
            cmp byte ptr [HeightRestriction::air_hike_dante], 1
            je AltRet
            jmp Code

        CheckNero:
            cmp byte ptr [HeightRestriction::air_hike_nero], 1
            je AltRet
        Code:
            comiss xmm0, [eax+0x00000158]
            jmp dword ptr [HeightRestriction::air_hike_jmp_ret]

        AltRet:
            jmp dword ptr [air_hike_jmp_alt]
    }
}

void HeightRestriction::on_gui_frame(int display) {
    if (display == 1) {
        if (ImGui::Checkbox(_("Height Restriction"), &toggle_nero)) {
            ToggleNero(toggle_nero);
        }
        ImGui::SameLine();
        help_marker(_("By default, certain moves are locked behind a height threshold. If you're below that threshold, they cannot be started. "
            "This removes that limit"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Air Hike Height Restriction"), &air_hike_nero);
    }
    else if (display == 2) {
        if (ImGui::Checkbox(_("Height Restriction"), &toggle_dante)) {
            ToggleDante(toggle_dante);
        }
        ImGui::SameLine();
        help_marker(_("By default, certain moves are locked behind a height threshold. If you're below that threshold, they cannot be started. "
            "This removes that limit"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Air Hike Height Restriction"), &air_hike_dante);
    }
}

std::optional<std::string> HeightRestriction::on_initialize() {
    if (!install_hook_offset(0x4038F1, hook_air_hike, &detour_air_hike, &air_hike_jmp_ret, 7)) {
		spdlog::error("Failed to init NoDtCooldown\n");
		return "Failed to init NoDtCooldown";
	}

    return Mod::on_initialize();
}

void HeightRestriction::on_config_load(const utility::Config& cfg) {
    toggle_nero = cfg.get<bool>("height_restriction_removed_nero").value_or(false);
    if (toggle_nero) ToggleNero(toggle_nero);
    toggle_dante = cfg.get<bool>("height_restriction_removed_dante").value_or(false);
    if (toggle_dante) ToggleDante(toggle_dante);

    air_hike_nero = cfg.get<bool>("airhike_height_restriction_nero").value_or(false);
    air_hike_dante = cfg.get<bool>("airhike_height_restriction_dante").value_or(false);
}

void HeightRestriction::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("height_restriction_removed_nero", toggle_nero);
    cfg.set<bool>("height_restriction_removed_dante", toggle_dante);

    cfg.set<bool>("airhike_height_restriction_nero", air_hike_nero);
    cfg.set<bool>("airhike_height_restriction_dante", air_hike_dante);
}

#endif
