
#include "LimitAdjust.hpp"

bool LimitAdjust::gun_sword = false;
bool LimitAdjust::enemy_step = false;
bool LimitAdjust::style = false;
bool LimitAdjust::target = false;

float LimitAdjust::limit = 0.0f;

std::optional<std::string> LimitAdjust::on_initialize() {
    return Mod::on_initialize();
}

naked void limitadjust_patch(void) {
	_asm {
		fld dword ptr [LimitAdjust::limit]
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
        patchenemystep.reset();
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

void LimitAdjust::on_gui_frame() {
    if (ImGui::Checkbox("Remove Sword & Gun Switch Limit", &gun_sword)) {
        f_sword_and_gun(gun_sword);
    }
    if (ImGui::Checkbox("Remove Enemy Step Limit", &enemy_step)) {
        f_enemy_step(enemy_step);
    }
    if (ImGui::Checkbox("Remove Style Switch Limit", &style)) {
        f_style(style);
    }
    if (ImGui::Checkbox("Remove Target Switch Limit", &target)) {
        f_target(target);
    }
}

void LimitAdjust::on_config_load(const utility::Config& cfg) {
    gun_sword = cfg.get<bool>("sword_switch_limits_removed").value_or(false);
    f_sword_and_gun(gun_sword);
    enemy_step = cfg.get<bool>("jc_limits_removed").value_or(false);
    f_enemy_step(enemy_step);
    style = cfg.get<bool>("style_switch_limits_removed").value_or(false);
    f_style(style);
    target = cfg.get<bool>("target_change_limit_removed").value_or(false);
    f_target(target);
}

void LimitAdjust::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("sword_switch_limits_removed", gun_sword);
    cfg.set<bool>("jc_limits_removed", enemy_step);
    cfg.set<bool>("style_switch_limits_removed", style);
    cfg.set<bool>("target_change_limit_removed", target);
}
