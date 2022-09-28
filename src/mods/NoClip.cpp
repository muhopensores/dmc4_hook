#include "NoClip.hpp"
#include "utility/MessageDisplay.hpp"
#include "Mutators.hpp"
#if 1
bool NoClip::mod_enabled{false};

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

void NoClip::on_gui_frame() {
    if (ImGui::Checkbox("Noclip", &mod_enabled)) {
        toggle(mod_enabled);
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
}

void NoClip::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("noclip", mod_enabled);
}

void NoClip::on_frame(fmilliseconds& dt) {
    //m_timer->tick(dt);
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
