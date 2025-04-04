
#include "LimitAdjust.hpp"

bool LimitAdjust::gun_sword = false;
bool LimitAdjust::enemy_step = false;
bool LimitAdjust::enemy_step_lower = false;
bool LimitAdjust::style = false;
bool LimitAdjust::target = false;
bool LimitAdjust::same_tick_style_change = false;
float LimitAdjust::limit = 0.0f;
float LimitAdjust::whirlingLimit = 14.0f;
std::optional<std::string> LimitAdjust::on_initialize() {
    return Mod::on_initialize();
}

naked void limitadjust_patch(void) {
	_asm {
		fld dword ptr [LimitAdjust::limit]
	}
}
naked void limitadjust_patch2(void) {
	_asm {
		fld dword ptr [LimitAdjust::whirlingLimit]
	}
}

void LimitAdjust::f_sword_and_gun(bool enable) {
    if (enable) {
        install_patch_offset(0x04045DE, patchsword, (char*)&limitadjust_patch, 6);
        install_patch_offset(0x0404625, patchgun, (char*)&limitadjust_patch, 6);
    }
    else {
        patchsword.reset();
        patchgun.reset();
    }
}

void LimitAdjust::f_enemy_step(bool enable) {
    if (enable) {
        install_patch_offset(0x0404A8E, patchenemystep, (char*)&limitadjust_patch, 6);
    }
    else {
        patchenemystep.reset(); // "\xD9\x81\x3C\x01\x00\x00"
    }
}

void LimitAdjust::f_enemy_step_lower(bool enable) {
    if (enable) {
        install_patch_offset(0x0404A8E, patchenemystep, (char*)&limitadjust_patch2, 6);
    }
    else {
        patchenemystep.reset(); // "\xD9\x81\x3C\x01\x00\x00"
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

void LimitAdjust::f_target(bool enable) {
    if (enable) {
        install_patch_offset(0x04208D7, patchtarget, "\x90\x90", 2);
    }
    else {
        patchtarget.reset();
    }
}

void LimitAdjust::f_same_tick_style_change(bool enable) {
    if (enable) {
        install_patch_offset(0x03B726F, patchsametickstylechange, "\x90\x90\x90\x90", 4);
    } else {
        patchsametickstylechange.reset();
    }
}

void LimitAdjust::on_gui_frame() {
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



    if (ImGui::Checkbox(_("Target Switch Limit"), &target)) {
        f_target(target);
    }
    ImGui::SameLine();
    help_marker(_("By default, the Target Switch button does nothing while holding the left stick (or WASD) in a direction. This removes "
                "that limit and allows you to target switch whenever"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Same Tick Style Change"), &same_tick_style_change)) {
        f_same_tick_style_change(same_tick_style_change);
    }
    ImGui::SameLine();
    help_marker(_("Style changes registered on the same tick as other inputs are ignored by default.\nOne bug has been found with this cheat "
                "so far: Every other time you perform Climax you'll spawn Discipline pins too"));

    if (ImGui::Checkbox(_("Enemy Step Limit"), &enemy_step)) {
        enemy_step_lower = false;
        f_enemy_step_lower(enemy_step_lower);
        f_enemy_step(enemy_step);
    }
    ImGui::SameLine();
    help_marker(_("By default, there is a short timer that must end before two Jump Cancels can be made consecutively. This cheat removes "
                "that timer and allows you to Jump Cancel as often as you want\nThis makes certain sequences much easier. If you're "
                "learning the game, I recommend not using this to save getting into bad habits"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Lower Enemy Step Limit"), &enemy_step_lower)) {
        enemy_step = false;
        f_enemy_step(enemy_step);
        f_enemy_step_lower(enemy_step_lower);
    }
    ImGui::SameLine();
    help_marker(_("Rather than removing the enemy step timer, reduce it from 20 to 14"));
}

void LimitAdjust::on_config_load(const utility::Config& cfg) {
    gun_sword = cfg.get<bool>("sword_switch_limits_removed").value_or(false);
    f_sword_and_gun(gun_sword);
    enemy_step = cfg.get<bool>("jc_limits_removed").value_or(false);
    f_enemy_step(enemy_step);
    enemy_step_lower = cfg.get<bool>("jc_limits_lowered").value_or(false);
    if (!enemy_step)
        f_enemy_step_lower(enemy_step_lower);
    style = cfg.get<bool>("style_switch_limits_removed").value_or(false);
    f_style(style);
    target = cfg.get<bool>("target_change_limit_removed").value_or(false);
    f_target(target);
    same_tick_style_change = cfg.get<bool>("same_tick_style_change_limit_removed").value_or(false);
    f_same_tick_style_change(same_tick_style_change);
}

void LimitAdjust::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("sword_switch_limits_removed", gun_sword);
    cfg.set<bool>("jc_limits_removed", enemy_step);
    cfg.set<bool>("jc_limits_lowered", enemy_step_lower);
    cfg.set<bool>("style_switch_limits_removed", style);
    cfg.set<bool>("target_change_limit_removed", target);
    cfg.set<bool>("same_tick_style_change_limit_removed", same_tick_style_change);
}
