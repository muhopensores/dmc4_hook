#include "ArcadeMode.hpp"

#include "sdk/uMissionMenu.hpp"
#include "sdk/uMissionStart.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sArea.hpp"
#include "sdk/aGame.hpp"

#include "mods/FastStart.hpp"

#include "utility/CommandLineArgs.hpp"
#include "AreaJump.hpp"

#define ARCADE_BP_DIFF 7

bool ArcadeMode::mod_enabled            = false;
bool ArcadeMode::launched_via_cmd       = false;
bool ArcadeMode::user_modified_settings = false;

uMissionMenu ArcadeMode::g_our_mission_menu{};
static int g_bp_floor = 1;

void uMissionMenu_our_set_mission_num(int mission) {
    if ((mission < 1) || (mission > 20)) {
        spdlog::warn("[ArcadeMode]: set_mission_num mission id outside of range");
        return;
    }
    ArcadeMode::g_our_mission_menu.hum = mission; 
    ArcadeMode::g_our_mission_menu.idk = mission; 
    ArcadeMode::g_our_mission_menu.dvh = mission; 
    ArcadeMode::g_our_mission_menu.sos = mission; 
    ArcadeMode::g_our_mission_menu.dmd = mission; 
    ArcadeMode::g_our_mission_menu.ldk = mission; 
    ArcadeMode::g_our_mission_menu.hoh = mission; 
    ArcadeMode::g_our_mission_menu.hah = mission; 
}

bool __fastcall uMissionStart_check_start_flag_sub_89F4C0(uMissionMenu* mission_struct) {
    if (ArcadeMode::mod_enabled || ArcadeMode::launched_via_cmd) {
        // uMissionMenu check
        if (mission_struct->vtable == 0xBDFD88) {
            mission_struct->cursor = ArcadeMode::g_our_mission_menu.cursor;

            mission_struct->hum = ArcadeMode::g_our_mission_menu.hum; // 0x0070
            mission_struct->idk = ArcadeMode::g_our_mission_menu.idk; // 0x006C
            mission_struct->dvh = ArcadeMode::g_our_mission_menu.dvh; // 0x0074
            mission_struct->sos = ArcadeMode::g_our_mission_menu.sos; // 0x0078
            mission_struct->dmd = ArcadeMode::g_our_mission_menu.dmd; // 0x007C
            mission_struct->ldk = ArcadeMode::g_our_mission_menu.ldk; // 0x0080
            mission_struct->hoh = ArcadeMode::g_our_mission_menu.hoh; // 0x0084
            mission_struct->hah = ArcadeMode::g_our_mission_menu.hah; // 0x0088

            mission_struct->character = ArcadeMode::g_our_mission_menu.character;

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
    if (ArcadeMode::mod_enabled || ArcadeMode::launched_via_cmd) {
        if (g_bp_floor != 1) {
            auto* stage = AreaJump::bp_stage(g_bp_floor);
            sArea* sap  = devil4_sdk::get_sArea();
            assert(sap);
            aGame* agp = sap->aGamePtr;
            assert(agp);
            agp->room_id  = stage->id;
            agp->bp_floor = g_bp_floor;
            agp->init_jump = 1;
        }

        return true;
    }

    return bp->flags == 2;
}

std::optional<std::string> install_hooks(ArcadeMode* arcade) {
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
        ImGui::BeginGroup();
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::Checkbox(_("Arcade"), &mod_enabled)) {
            user_modified_settings = true;

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
            "You can also start dmc4 with steam launch options, using something like\n"
            "-arcade -difficulty 3 -mission 12 (DMD, M12), or\n"
            "-arcade -character 6 -bp 4 (Nero, BP floor 4)"));

        if (mod_enabled) {
            ImGui::Indent(lineIndent);
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

            if (ImGui::Combo(_("Character"), (int*)&g_our_mission_menu.character, char_names, 8)) {
                user_modified_settings = true;
            }

            if (g_our_mission_menu.cursor == ARCADE_BP_DIFF) {
                if (ImGui::InputInt(_("Floor"), &g_bp_floor, 1, 10)) {
                    user_modified_settings = true;
                    if (g_bp_floor < 1) {
                        g_bp_floor = 1;
                    }
                    if (g_bp_floor > 101) {
                        g_bp_floor = 101;
                    }
                }
            } else {
                if (ImGui::SliderInt(_("Mission"), (int*)&g_our_mission_menu.hum, 1, 20)) {
                    user_modified_settings = true;
                    uMissionMenu_our_set_mission_num(g_our_mission_menu.hum);
                }
            }

            static const char* difficulty_names[9]{
                __("Human"),                 // 0
                __("Devil Hunter"),          // 1
                __("Son Of Sparda"),         // 2
                __("Dante Must Die"),        // 3
                __("Legendary Dark Knight"), // 4
                __("Heaven Or Hell (SOS)"),  // 5
                __("Hell And Hell (SOS)"),   // 6
                __("Bloody Palace"),         // 7
                __("Story Theater"),         // 8
                // "I assume 10 would be training but they removed it, just black screens",
            };

            if (ImGui::Combo(_("Difficulty"), (int*)&g_our_mission_menu.cursor, difficulty_names, 9)) {
                user_modified_settings = true;
            }

            ImGui::Unindent(lineIndent);
        }
        ImGui::PopItemWidth();
        ImGui::EndGroup();
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
        FastStart::set_bools(true);

        launched_via_cmd  = true;
        mod_enabled       = true;

        int arg_char = utility::get_argument("-character");
        if (arg_char > 0) {
            g_our_mission_menu.character = arg_char;
        }

        g_our_mission_menu.cursor = 3;
        int difficulty = utility::get_argument("-difficulty");
        if (difficulty > 0) {
            g_our_mission_menu.cursor = difficulty;
        }

        if (utility::check_argument("-bp")) {
            g_our_mission_menu.cursor = ARCADE_BP_DIFF;
            int floor = utility::get_argument("-bp");
            if (floor > 1) {
                g_bp_floor = floor;
            }
        }

        int mission = utility::get_argument("-mission");
        if (mission > 0) {
            uMissionMenu_our_set_mission_num(mission);
        }
    }

    if (mod_enabled || launched_via_cmd) {
        auto res = install_hooks(this);
        if (res.has_value()) {
            spdlog::error("error initializing arcade mode {}", res.value());
        }
    }
}

void ArcadeMode::on_config_save(utility::Config& cfg) {
    if (launched_via_cmd && !user_modified_settings) {
        return;
    }
    cfg.set<bool>("ArcadeMode", mod_enabled);
    cfg.set<int>("ArcadeMode.difficulty", g_our_mission_menu.cursor);
    cfg.set<int>("ArcadeMode.character", g_our_mission_menu.character);
    cfg.set<int>("ArcadeMode.mission", g_our_mission_menu.hum);
}
