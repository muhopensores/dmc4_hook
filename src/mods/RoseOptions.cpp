#include "RoseOptions.hpp"

#if 1
static bool enabled_no_despawn_enemy = false;
static bool enabled_no_despawn_object = false;
static bool rose_infinite_timer = false;

std::optional<std::string> RoseOptions::on_initialize() {
    return Mod::on_initialize();
}

void RoseOptions::toggle_disable_rose_despawn_on_hitting_enemy(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435944, patch1, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch1.reset();
    }
}

void RoseOptions::toggle_disable_rose_despawn_on_hitting_object(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435922, patch2, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch2.reset();
    }
}

void RoseOptions::toggle_rose_infinite_timer(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435937, patch3, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch3.reset();
    }
}

void RoseOptions::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Rose Survives Enemies"), &enabled_no_despawn_enemy)) {
        toggle_disable_rose_despawn_on_hitting_enemy(enabled_no_despawn_enemy);
    }
    ImGui::SameLine();
    help_marker(_("Rose will hit the enemy and then continue past the enemy"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Rose Survives Objects"), &enabled_no_despawn_object)) {
        toggle_disable_rose_despawn_on_hitting_object(enabled_no_despawn_object);
    }
    ImGui::SameLine();
    help_marker(_("Rose will sit on whatever surface it lands on until its timer expires"));

    if (ImGui::Checkbox(_("Infinite Rose Timer"), &rose_infinite_timer)) {
        toggle_rose_infinite_timer(rose_infinite_timer);
    }
    ImGui::SameLine();
    help_marker(_("Rose will only despawn when colliding with something or having another placed"));
}

void RoseOptions::on_config_load(const utility::Config& cfg) {
    enabled_no_despawn_enemy = cfg.get<bool>("no_despawn_rose_enemy").value_or(false);
    toggle_disable_rose_despawn_on_hitting_enemy(enabled_no_despawn_enemy);
    enabled_no_despawn_object = cfg.get<bool>("no_despawn_rose_object").value_or(false);
    toggle_disable_rose_despawn_on_hitting_object(enabled_no_despawn_object);
    rose_infinite_timer = cfg.get<bool>("rose_infinite_timer").value_or(false);
    toggle_rose_infinite_timer(rose_infinite_timer);
};

void RoseOptions::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_despawn_rose_enemy", enabled_no_despawn_enemy);
    cfg.set<bool>("no_despawn_rose_object", enabled_no_despawn_object);
    cfg.set<bool>("rose_infinite_timer", rose_infinite_timer);
};

#endif
