#include "ArcadeMode.hpp"

#include "sdk/uMissionMenu.hpp"
#include "sdk/uMissionStart.hpp"

#include "utility/CommandLineArgs.hpp"

static bool g_arcade = false;

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
    if(g_arcade) { 
        // uMissionMenu check
        if (mission_struct->vtable == 0xBDFD88) {
            mission_struct->cursor = g_our_mission_menu.cursor;

            mission_struct->hum = g_our_mission_menu.hum;        // 0x0070
            mission_struct->idk = g_our_mission_menu.idk;        // 0x006C
            mission_struct->dvh = g_our_mission_menu.dvh;        // 0x0074
            mission_struct->sos = g_our_mission_menu.sos;        // 0x0078
            mission_struct->dmd = g_our_mission_menu.dmd;        // 0x007C
            mission_struct->ldk = g_our_mission_menu.ldk;        // 0x0080
            mission_struct->hoh = g_our_mission_menu.hoh;        // 0x0084
            mission_struct->hah = g_our_mission_menu.hah;        // 0x0088

            mission_struct->character = g_our_mission_menu.character;
        }
        return true; 
    }
    return mission_struct->flags == 4;
}

bool __fastcall uBloodyPalaceStart_check_start_sub_854050(uBloodyPalaceStart* bp) {
    if(g_arcade) { return true; }
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

std::optional<std::string> ArcadeMode::on_initialize() {
    g_our_mission_menu.cursor = 7; // bp start
    uMissionMenu_our_set_mission_num(12);

    bool has_arg = utility::check_argument("-arcade");
    if (has_arg) {
        g_arcade = true;
        install_hooks(this);
    }

    uint32_t arg_char = utility::get_argument("-character");
    if (arg_char > 0) {
        g_our_mission_menu.character = arg_char;
    }

    uint32_t arg_cursor = utility::get_argument("-difficulty");
    if (arg_cursor > 0) {
        g_our_mission_menu.cursor = arg_cursor;
    }

    int arg_mission = utility::get_argument("-mission");
    if (arg_mission > 0) {
        uMissionMenu_our_set_mission_num(arg_mission);
    }

    return Mod::on_initialize();
}

void ArcadeMode::on_config_load(const utility::Config& cfg){
    bool arcade = cfg.get<bool>("ArcadeMode").value_or(false);
    if (arcade) {
        g_arcade = true;
        auto res = install_hooks(this);
        if (res.has_value()) {
            spdlog::error("error initializing arcade mode {}", res.value());
        }
    }
}

void ArcadeMode::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ArcadeMode", g_arcade);
}

void ArcadeMode::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Enable arcade mode"), &g_arcade)) {
            if (g_arcade) {
                auto res = install_hooks(this);
                if (res.has_value()) {
                    spdlog::error("error initializing arcade mode {}", res.value());
                }
            } else {
                hook_bpstart.reset();
                hook_msel_mstart.reset();
            }
        }
        if (g_arcade) {
            // TODO: draw g_our_mission_menu with imgui
        }
    }
}
