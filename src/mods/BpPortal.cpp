#include "BpPortal.hpp"
#include "DeltaTime.hpp"
#include "EnemySpawn.hpp"

bool BpPortal::mod_enabled{false};
static BpPortal* g_mod{nullptr};

static void on_timer_callback() {
    BpPortal::mod_enabled = !BpPortal::mod_enabled;
    g_mod->toggle(BpPortal::mod_enabled);
    // DISPLAY_MESSAGE("BP Portal / Red Seals Closed"); // debug
    BpPortal::mod_enabled = false;
    g_mod->toggle(BpPortal::mod_enabled);
}

std::optional<std::string> BpPortal::on_initialize() {
    g_mod    = this;
    m_timer  = new utility::Timer(0.5f, on_timer_callback);
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_F4, "Open BP Portal / Red Seal", "red_seal_and_bp_portal_key"));
    return Mod::on_initialize();
}

void BpPortal::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x004E1E0, patch, "\x75\x07", 2);
    } else {
        patch.reset();
        // install_patch_offset(0x004E1E0, patch, "\x74\x07", 2);
    }
}

void BpPortal::on_gui_frame() {
    if (ImGui::Checkbox("Ignore Red Seal Kill Requirements", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Sealed doors and portals open instantly");
}

void BpPortal::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("instant_bp_portal").value_or(false);
    toggle(mod_enabled);
};

void BpPortal::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("instant_bp_portal", mod_enabled);
};

void BpPortal::on_frame(fmilliseconds& dt) {
    m_timer->tick(dt);
};

void BpPortal::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        if (!mod_enabled) {
            DISPLAY_MESSAGE("BP Portal / Red Seals Opened");
            mod_enabled = true;
            toggle(mod_enabled);
            m_timer->start();
        }
    }
}
