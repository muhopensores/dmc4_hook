#include "ArcadeMode.hpp"

#include "sdk/uMissionMenu.hpp"
#include "sdk/uMissionStart.hpp"

#include "utility/CommandLineArgs.hpp"

bool ArcadeMode::mod_enabled = false;

static uMissionMenu g_our_mission_menu {};

void uMissionMenu_our_set_mission_num(int mission) {
    if ((mission < 1) || (mission > 20)) {
        spdlog::warn("[ArcadeMode]: set_mission_num mission id outside of range");
        return;
    }
    g_our_mission_menu.hum = mission; 
    g_our_mission_menu.idk = mission; 
    g_our_mission_menu.dvh = mission; 
    g_our_mission_menu.sos = mission; 
    g_our_mission_menu.dmd = mission; 
    g_our_mission_menu.ldk = mission; 
    g_our_mission_menu.hoh = mission; 
    g_our_mission_menu.hah = mission; 
}

bool __fastcall uMissionStart_check_start_flag_sub_89F4C0(uMissionMenu* mission_struct) {
    if (ArcadeMode::mod_enabled) { 
        // uMissionMenu check
        if (mission_struct->vtable == 0xBDFD88) {
            mission_struct->cursor = g_our_mission_menu.cursor;

            mission_struct->hum = g_our_mission_menu.hum; // 0x0070
            mission_struct->idk = g_our_mission_menu.idk; // 0x006C
            mission_struct->dvh = g_our_mission_menu.dvh; // 0x0074
            mission_struct->sos = g_our_mission_menu.sos; // 0x0078
            mission_struct->dmd = g_our_mission_menu.dmd; // 0x007C
            mission_struct->ldk = g_our_mission_menu.ldk; // 0x0080
            mission_struct->hoh = g_our_mission_menu.hoh; // 0x0084
            mission_struct->hah = g_our_mission_menu.hah; // 0x0088

            mission_struct->character = g_our_mission_menu.character;

            return true;
        }
        // uMissionStart check
        if (mission_struct->vtable == 0xBE0AC8) {
            return true;
        }
    }
    return mission_struct->flags == 4;
}

bool __fastcall uBloodyPalaceStart_check_start_sub_854050(uBloodyPalaceStart* bp) {
    if (ArcadeMode::mod_enabled) {
        return true;
    }
    return bp->flags == 2;
}

std::optional<std::string> install_hooks(ArcadeMode* arcade) {

    if(arcade->hook_bpstart || arcade->hook_msel_mstart) { return std::nullopt; }

    uintptr_t ass;
    if (!arcade->install_hook_absolute(0x89F4C0, arcade->hook_msel_mstart, uMissionStart_check_start_flag_sub_89F4C0, &ass, 5)) {
        return "failed to install uMissionStart hook";
    }
    if (!arcade->install_hook_absolute(0x854050, arcade->hook_bpstart, uBloodyPalaceStart_check_start_sub_854050, &ass, 5)) {
        return "failed to install uBloodyPalaceStart hook";
    }

    return std::nullopt;
}

void ArcadeMode::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Arcade"), &mod_enabled)) {
            if (mod_enabled) {
                auto res = install_hooks(this);
                if (res.has_value()) {
                    spdlog::error("error initializing arcade mode {}", res.value());
                }
            } else {
                hook_bpstart.reset();
                hook_msel_mstart.reset();
            }
        }
        ImGui::SameLine();
        help_marker(_("When opening the game or returning to the mission select screen, the game will load these settings\n"
            "You can also start dmc4 with command line arguments, using something like\n"
            "`DevilMayCry4_DX9.exe -arcade -difficulty 3 -mission 12` (DMD, M12), or\n"
            "`DevilMayCry4_DX9.exe -arcade -character 4 -difficulty 7` (Dante, BP)"));

        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            if (g_our_mission_menu.cursor != 7) {
                if (ImGui::SliderInt(_("Mission"), (int*)&g_our_mission_menu.hum, 1, 20)) {
                    uMissionMenu_our_set_mission_num(g_our_mission_menu.hum);
                }
            }

            static const char* char_names[8]{
                //_("Dante"),
                // __("Dante - Auto"),
                // __("Super Dante"),
                // __("Super Dante - Auto"),
                // __("Nero"), // 6?
                // __("Nero - Auto"),
                // __("Super Nero"),
                // __("Super Nero - Auto"),
                __("Dante"),
                __("Dante"),
                __("Dante"),
                __("Dante"),
                __("Nero"),
                __("Nero"),
                __("Nero"),
                __("Nero"),
            };

            if (g_our_mission_menu.cursor == 7) {
                ImGui::Combo(_("Character"), (int*)&g_our_mission_menu.character, char_names, 8);
            }

            static const char* difficulty_names[9] {
                __("Human"), // 0
                __("Devil Hunter"), // 1
                __("Son Of Sparda"), // 2
                __("Dante Must Die"), // 3
                __("Legendary Dark Knight"), // 4
                __("Heaven Or Hell (SOS)"), // 5
                __("Hell And Hell (SOS)"),  // 6
                __("Bloody Palace"), // 7
                __("Story Theater"), // 8
                // "I assume 10 would be training but they removed it, just black screens",
            };

            ImGui::Combo(_("Difficulty"), (int*)&g_our_mission_menu.cursor, difficulty_names, 9);

            ImGui::Unindent(lineIndent);
        }
    }
}

std::optional<std::string> ArcadeMode::on_initialize() {
    return Mod::on_initialize();
}

void ArcadeMode::on_config_load(const utility::Config& cfg) {
    mod_enabled                  = cfg.get<bool>("ArcadeMode").value_or(false);
    g_our_mission_menu.cursor    = cfg.get<int>("ArcadeMode.difficulty").value_or(7);
    g_our_mission_menu.character = cfg.get<int>("ArcadeMode.character").value_or(6);
    g_our_mission_menu.hum       = cfg.get<int>("ArcadeMode.mission").value_or(12);
    uMissionMenu_our_set_mission_num(g_our_mission_menu.hum);

    if (utility::check_argument("-arcade")) {
        mod_enabled = true;
        uint32_t arg_char = utility::get_argument("-character");
        if (arg_char > 0)
            g_our_mission_menu.character = arg_char;
        uint32_t arg_cursor = utility::get_argument("-difficulty");
        if (arg_cursor > 0)
            g_our_mission_menu.cursor = arg_cursor;
        int arg_mission = utility::get_argument("-mission");
        if (arg_mission > 0)
            uMissionMenu_our_set_mission_num(arg_mission);
    }

    if (mod_enabled) {
        auto res = install_hooks(this);
        if (res.has_value()) {
            spdlog::error("error initializing arcade mode {}", res.value());
        }
    }
}

void ArcadeMode::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ArcadeMode", mod_enabled);
    cfg.set<int>("ArcadeMode.difficulty", g_our_mission_menu.cursor);
    cfg.set<int>("ArcadeMode.character", g_our_mission_menu.character);
    cfg.set<int>("ArcadeMode.mission", g_our_mission_menu.hum);
}
