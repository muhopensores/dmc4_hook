#include "EnemyDT.hpp"

bool EnemyDT::mod_enabled_no_dt{ false };
bool EnemyDT::mod_enabled_instant_dt{ false };

std::optional<std::string> EnemyDT::on_initialize() {
    return Mod::on_initialize();
}

void EnemyDT::toggle_no_dt(bool enable) {
    if (enable) {
        install_patch_offset(0x03309DD, patch1, "\x75\x0D", 2);
    }
    else {
        patch1.reset();
    }
}
void EnemyDT::toggle_instant_dt(bool enable) {
    if (enable) {
        // install_patch_offset(0x03309A1, patch2, "\xEB\x1f", 2); // forces DT on any difficulty
        install_patch_offset(0x03309DF, patch3, "\x0f\x57\xc0\x90\x90", 5);
    }
    else {
        // patch2.reset();
        patch3.reset(); // movss xmm0, [esi+00001488]
    }
}

void EnemyDT::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Enemies Don't DT"), &mod_enabled_no_dt)) {
        mod_enabled_instant_dt = 0;              // turn off other mod
        toggle_instant_dt(mod_enabled_instant_dt); // update other mod
        toggle_no_dt(mod_enabled_no_dt);
    }
    ImGui::SameLine();
    help_marker(_("Disables enemy Devil Trigger on every difficulty"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Enemies DT Instantly"), &mod_enabled_instant_dt)) {
        mod_enabled_no_dt = 0;                   // turn off other mod
        toggle_no_dt(mod_enabled_no_dt);           // update other mod
        toggle_instant_dt(mod_enabled_instant_dt);
    }
    ImGui::SameLine();
    help_marker(_("If you're playing a difficulty where enemies have access to Devil Trigger, they will activate it instantly"));
}

void EnemyDT::on_config_load(const utility::Config& cfg) {
    mod_enabled_no_dt = cfg.get<bool>("enemy_no_dt").value_or(false);
    toggle_no_dt(mod_enabled_no_dt);
    mod_enabled_instant_dt = cfg.get<bool>("enemy_instant_dt").value_or(false);
    toggle_instant_dt(mod_enabled_instant_dt);
}

void EnemyDT::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_no_dt", mod_enabled_no_dt);
    cfg.set<bool>("enemy_instant_dt", mod_enabled_instant_dt);
}
