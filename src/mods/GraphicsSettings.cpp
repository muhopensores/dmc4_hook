#include "GraphicsSettings.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/uShadow.hpp"
#include "sdk/sUnit.hpp"

bool GraphicsSettings::disable_prop_fade_enabled = false;
bool GraphicsSettings::disable_bullet_casing_fade_enabled = false;
bool GraphicsSettings::shadow_upgrade_enabled = false;
bool GraphicsSettings::water_upgrade_enabled  = false;
bool GraphicsSettings::disable_bloom_enabled = false;
bool GraphicsSettings::disable_lightshafts_enabled = false;

void GraphicsSettings::prop_fade_toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x4634D7, prop_fade_patch1, "\xE9\x78\x01\x00\x00\x90", 6);
    }
    else {
        prop_fade_patch1.reset();
    }
}

void GraphicsSettings::bullet_casing_fade_toggle(bool enable) {
    if (enable) {
        install_patch_offset(0xFAA19, bullet_casing_fade_patch1, "\xEB\x0D", 2); // stops cartridges being destroyed after timer
        install_patch_offset(0xFAA41, bullet_casing_fade_patch2, "\x90\x90", 2); // stops cartridges being destroyed after timer
        install_patch_offset(0xFAF76, bullet_casing_fade_patch3, "\x90\x90\x90\x90\x90\x90", 6); // stops cartridges being destroyed after 4 bounces
        install_patch_offset(0xFAA2F, bullet_casing_fade_patch4, "\x90\x90\x90\x90\x90", 5); // stops cartridges spinning
    }
    else {
        bullet_casing_fade_patch1.reset();
        bullet_casing_fade_patch2.reset();
        bullet_casing_fade_patch3.reset();
        bullet_casing_fade_patch4.reset();
    }
}

void GraphicsSettings::shadowres_toggle(bool enable) {
    if (enable) { // reset mid gameplay
        install_patch_offset(0x67D3B9, shadows_patch1, "\x08", 1); // patch cmp eax, 400 to cmp eax, 800
        install_patch_offset(0x67D3C0, shadows_patch2, "\x08", 1); // patch mov eax, 400 to mov eax, 800
    }
    else {
        shadows_patch1.reset();
        shadows_patch2.reset();
        sUnit* sUnit = devil4_sdk::get_sUnit();
        if (!sUnit) { return; };
        uShadow* shadow = (uShadow*)sUnit->mMoveLine[1].mBottom;
        if (!shadow) { return; };
        shadow->resolution = 0;
    }
}

void GraphicsSettings::waterres_toggle(bool enable) {
    if (enable) { // reset mid gameplay
        install_patch_offset(0x6BF82B, water_patch1, "\x08", 1); // push 0x800 (2048)
        install_patch_offset(0x6BF830, water_patch2, "\x08", 1); // push 0x800 (2048)
    }
    else {
        water_patch1.reset(); // push 0x200 (512)
        water_patch2.reset(); // push 0x200 (512)
    }
}

void GraphicsSettings::bloom_toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x52E1B8, bloom_patch1, "\xEB", 1); // jmp
    } else {
        bloom_patch1.reset(); // je
    }
}

void GraphicsSettings::lightshafts_toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x52E1F1, lightshafts_patch1, "\xEB", 1); // jmp
    } else {
        lightshafts_patch1.reset(); // je
    }
}

// void on_frame(fmilliseconds& dt) {}

void GraphicsSettings::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Disable Prop Fade"), &disable_prop_fade_enabled)) {
            prop_fade_toggle(disable_prop_fade_enabled);
        }

        ImGui::SameLine(sameLineWidth);

        if (ImGui::Checkbox(_("Disable Bullet Casing Fade"), &disable_bullet_casing_fade_enabled)) {
            bullet_casing_fade_toggle(disable_bullet_casing_fade_enabled);
        }
        ImGui::SameLine();
        help_marker(_("This will kill your fps if you spawn too many\nAlso stops cartridges from spinning so they don't endlessly spin while on the floor"));

        if (ImGui::Checkbox(_("Increased Shadow Resolution"), &shadow_upgrade_enabled)) {
            shadowres_toggle(shadow_upgrade_enabled);
        }

        ImGui::SameLine(sameLineWidth);

        if (ImGui::Checkbox(_("Increased Water Resolution"), &water_upgrade_enabled)) {
            waterres_toggle(water_upgrade_enabled);
        }

        if (ImGui::Checkbox(_("Disable Bloom"), &disable_bloom_enabled)) {
            bloom_toggle(disable_bloom_enabled);
        }

        ImGui::SameLine(sameLineWidth);

        if (ImGui::Checkbox(_("Disable Lightshafts"), &disable_lightshafts_enabled)) {
            lightshafts_toggle(disable_lightshafts_enabled);
        }
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> GraphicsSettings::on_initialize() {
    /*if (!install_hook_offset(0x67D3C9, hook1, &detour1, &jmp_ret1, 8)) {
        spdlog::error("Failed to init GraphicsSettings mod 1\n");
        return "Failed to init GraphicsSettings mod 1";
	}*/

    return Mod::on_initialize();
}

void GraphicsSettings::on_config_load(const utility::Config& cfg){
    disable_prop_fade_enabled = cfg.get<bool>("DisablePropFade").value_or(false);
    if (disable_prop_fade_enabled) prop_fade_toggle(disable_prop_fade_enabled);

    disable_bullet_casing_fade_enabled = cfg.get<bool>("DisableBulletCasingFade").value_or(false);
    if (disable_bullet_casing_fade_enabled) bullet_casing_fade_toggle(disable_bullet_casing_fade_enabled);

    shadow_upgrade_enabled = cfg.get<bool>("shadow_upgrade_enabled").value_or(false);
    if (shadow_upgrade_enabled) shadowres_toggle(shadow_upgrade_enabled);

    water_upgrade_enabled = cfg.get<bool>("water_upgrade_enabled").value_or(false);
    if (water_upgrade_enabled) waterres_toggle(water_upgrade_enabled);

    disable_bloom_enabled = cfg.get<bool>("disable_bloom_enabled").value_or(false);
    if (disable_bloom_enabled) bloom_toggle(disable_bloom_enabled);

    disable_lightshafts_enabled = cfg.get<bool>("disable_lightshafts_enabled").value_or(false);
    if (disable_lightshafts_enabled) lightshafts_toggle(disable_lightshafts_enabled);
}

void GraphicsSettings::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("DisablePropFade", disable_prop_fade_enabled);
    cfg.set<bool>("DisableBulletCasingFade", disable_bullet_casing_fade_enabled);
    cfg.set<bool>("shadow_upgrade_enabled", shadow_upgrade_enabled);
    cfg.set<bool>("water_upgrade_enabled", water_upgrade_enabled);
    cfg.set<bool>("disable_bloom_enabled", disable_bloom_enabled);
    cfg.set<bool>("disable_lightshafts_enabled", disable_lightshafts_enabled);
}
