#include "NoClip.hpp"
#include "utility/MessageDisplay.hpp"
#if 1
bool NoClip::mod_enabled{false};
bool NoClip::mod_enabled_2{false};
bool player_lock_y_pos = false;
float player_y_backup  = 0.0f;

constexpr uintptr_t static_mediator_ptr = 0x00E558B8; // DevilMayCry4_DX9.exe+A558B8

static void on_timer_callback() {
    NoClip::mod_enabled = !NoClip::mod_enabled;
}

std::optional<std::string> NoClip::on_initialize() {
    MutatorRegistry::define("NoClip")
        .description("hehe")
        .on_init([&] {
            mod_enabled = true;
            toggle(mod_enabled);
        })
        .set_timer(30.0, [&] {
            mod_enabled = false;
            toggle(mod_enabled);
        });
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_F5, "NoClip", "noclip_key"));
    return Mod::on_initialize();
}

void NoClip::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x115BC4, patch1, "\x90\x90\x90", 3);
        install_patch_offset(0x10FA06, patch2, "\x01", 1);
    } else {
        patch1.reset();
        patch2.reset();
    }
}

void NoClip::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x4A8F3, patch3, "\x90\x90", 2);
    } else {
        patch3.reset();
    }
}

void NoClip::on_gui_frame() {
    if (ImGui::Checkbox(_("Noclip"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("You and enemies will ignore walls"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Disable Teleport Planes"), &mod_enabled_2)) {
        toggle2(mod_enabled_2);
    }
    ImGui::SameLine();
    help_marker(_("Disable the planes that teleport you back to the stage when you noclip out of bounds"));
    if (ImGui::Checkbox(_("Lock Y Pos"), &player_lock_y_pos)) {
        if (player_lock_y_pos) {
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player_y_backup = player->m_pos[1];
            }
        }
    }
    ImGui::SameLine();
    help_marker(_("Lock your height to explore beyond the environment more efficiently"));
}

void NoClip::on_frame(fmilliseconds& dt) {
    if (player_lock_y_pos) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            player->m_pos[1] = player_y_backup;
        }
    }
}

#if 0
void NoClip::on_twitch_command(std::size_t hash) {
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_command);
    if (hash == m_command) {
        mod_enabled = !mod_enabled;
        toggle(mod_enabled);
    }
}
#endif

void NoClip::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("noclip").value_or(false);
    toggle(mod_enabled);
    mod_enabled_2 = cfg.get<bool>("disable_teleport_planes").value_or(false);
    toggle2(mod_enabled_2);
}

void NoClip::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("noclip", mod_enabled);
    cfg.set<bool>("disable_teleport_planes", mod_enabled_2);
}

void NoClip::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        if (mod_enabled) {
            DISPLAY_MESSAGE("Noclip Off");
        } else {
            DISPLAY_MESSAGE("Noclip On");
        }
        mod_enabled = !mod_enabled;
        toggle(mod_enabled);
    }
}

#endif
