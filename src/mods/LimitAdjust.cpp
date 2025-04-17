#if 1
#include "LimitAdjust.hpp"

// shared
bool LimitAdjust::target_switch_enabled_nero = false;
bool LimitAdjust::target_switch_enabled_dante = false;
uintptr_t LimitAdjust::target_switch_ret = NULL;

bool LimitAdjust::enemy_step_enabled_nero = false;
bool LimitAdjust::enemy_step_enabled_dante = false;
uintptr_t LimitAdjust::enemy_step_ret = NULL;

// dante
float LimitAdjust::limit = 0.0f;
bool LimitAdjust::gun_sword = false;
bool LimitAdjust::style = false;
bool LimitAdjust::same_tick_style_change = false;


static constexpr uintptr_t target_switch_alt = 0x8208D9;
naked void detour_target_switch() {
    _asm {
            cmp byte ptr [edi+0x1494], 1 // controller id nero
            je CheckNero
            cmp byte ptr [LimitAdjust::target_switch_enabled_dante], 1
            je NewCode
            jmp Code

        CheckNero:
            cmp byte ptr [LimitAdjust::target_switch_enabled_nero], 1
            jne Code
        NewCode:
			jmp dword ptr [target_switch_alt] // skip conditional

        Code:
            cmp [edi+0x0000141C], ebx
            jmp dword ptr [LimitAdjust::target_switch_ret]
    }
}

naked void detour_enemy_step() {
    _asm {
            cmp byte ptr [edi+0x1494], 1 // controller id nero
            je CheckNero
            cmp byte ptr [LimitAdjust::enemy_step_enabled_dante], 1
            je NewCode
            jmp Code

        CheckNero:
            cmp byte ptr [LimitAdjust::enemy_step_enabled_nero], 1
            jne Code
        NewCode:
            fld dword ptr [LimitAdjust::limit]
            jmp RetCode

        Code:
            fld dword ptr [ecx+0x0000013C]
        RetCode:
            jmp dword ptr [LimitAdjust::enemy_step_ret]
    }
}

naked void LimitAdjust_patch(void) {
    _asm {
        fld dword ptr [LimitAdjust::limit]
    }
}

// dante
void LimitAdjust::f_sword_and_gun(bool enable) {
    if (enable) {
        install_patch_offset(0x04045DE, patchsword, (char*)&LimitAdjust_patch, 6);
        install_patch_offset(0x0404625, patchgun, (char*)&LimitAdjust_patch, 6);
    }
    else {
        patchsword.reset();
        patchgun.reset();
    }
}

void LimitAdjust::f_style(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6DC7, patchstyle, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patchstyle.reset();
    }
}

void LimitAdjust::f_same_tick_style_change(bool enable) {
    if (enable) {
        install_patch_offset(0x03B726F, patchsametickstylechange, "\x90\x90\x90\x90", 4);
    } else {
        patchsametickstylechange.reset();
    }
}


void LimitAdjust::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("Target Switch Limit"), &target_switch_enabled_nero);
        ImGui::SameLine();
        help_marker(_("By default, the Target Switch button does nothing while holding the left stick (or WASD) in a direction. This removes "
            "that limit and allows you to target switch whenever"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Enemy Step Limit"), &enemy_step_enabled_nero);
        ImGui::SameLine();
        help_marker(_("By default, there is a short timer that must end before two Jump Cancels can be made consecutively. This cheat removes "
            "that timer and allows you to Jump Cancel as often as you want\nThis makes certain sequences much easier. If you're "
            "learning the game, I recommend not using this to save getting into bad habits"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Target Switch Limit"), &target_switch_enabled_dante);
        ImGui::SameLine();
        help_marker(_("By default, the Target Switch button does nothing while holding the left stick (or WASD) in a direction. This removes "
            "that limit and allows you to target switch whenever"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Enemy Step Limit"), &enemy_step_enabled_dante);
        ImGui::SameLine();
        help_marker(_("By default, there is a short timer that must end before two Jump Cancels can be made consecutively. This cheat removes "
            "that timer and allows you to Jump Cancel as often as you want\nThis makes certain sequences much easier. If you're "
            "learning the game, I recommend not using this to save getting into bad habits"));

        if (ImGui::Checkbox(_("Sword & Gun Switch Limit"), &gun_sword)) {
            f_sword_and_gun(gun_sword);
        }
        ImGui::SameLine();
        help_marker(_("By default, there are short timers that must end before two sword or gun switches can be made consecutively. Without this "
                    "timer, you can swap to distant weapons quickly and more easily do strings like Shotgun > Enemy Step > Ebony & Ivory shot"));
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Style Switch Limit"), &style)) {
            f_style(style);
        }
        ImGui::SameLine();
        help_marker(_("By default, there is a short timer that must end before two style switches can be made consecutively. This cheat removes "
                    "that timer and allows you to Style Switch as often as you want"));

        if (ImGui::Checkbox(_("Same Tick Style Change"), &same_tick_style_change)) {
            f_same_tick_style_change(same_tick_style_change);
        }
        ImGui::SameLine();
        help_marker(_("Style changes registered on the same tick as other inputs are ignored by default.\nOne bug has been found with this cheat "
                    "so far: Every other time you perform Climax you'll spawn Discipline pins too"));
    }
}

std::optional<std::string> LimitAdjust::on_initialize() {
    if (!install_hook_offset(0x4208D1, hook_target_switch, &detour_target_switch, &target_switch_ret, 6)) {
		spdlog::error("Failed to init LimitAdjust 1\n");
		return "Failed to init LimitAdjust 1";
	}
    if (!install_hook_offset(0x404A8E, hook_enemy_step, &detour_enemy_step, &enemy_step_ret, 6)) {
		spdlog::error("Failed to init LimitAdjust 2\n");
		return "Failed to init LimitAdjust 2";
	}
    return Mod::on_initialize();
}

void LimitAdjust::on_config_load(const utility::Config& cfg) {
    // shared
    enemy_step_enabled_nero = cfg.get<bool>("jc_limits_removed_nero").value_or(false);
    enemy_step_enabled_dante = cfg.get<bool>("jc_limits_removed_dante").value_or(false);
    target_switch_enabled_nero = cfg.get<bool>("target_switch_limit_removed_nero").value_or(false);
    target_switch_enabled_dante = cfg.get<bool>("target_switch_limit_removed_dante").value_or(false);

    // dante
    gun_sword = cfg.get<bool>("sword_switch_limits_removed").value_or(false);
    f_sword_and_gun(gun_sword);
    style = cfg.get<bool>("style_switch_limits_removed").value_or(false);
    f_style(style);
    same_tick_style_change = cfg.get<bool>("same_tick_style_change_limit_removed").value_or(false);
    f_same_tick_style_change(same_tick_style_change);
}

void LimitAdjust::on_config_save(utility::Config& cfg) {
    // shared
    cfg.set<bool>("jc_limits_removed_nero", enemy_step_enabled_nero);
    cfg.set<bool>("jc_limits_removed_dante", enemy_step_enabled_dante);
    cfg.set<bool>("target_switch_limit_removed_nero", target_switch_enabled_nero);
    cfg.set<bool>("target_switch_limit_removed_dante", target_switch_enabled_dante);

    // dante
    cfg.set<bool>("sword_switch_limits_removed", gun_sword);
    cfg.set<bool>("style_switch_limits_removed", style);
    cfg.set<bool>("same_tick_style_change_limit_removed", same_tick_style_change);
}

#endif
