#include "modHideHud.hpp"

bool HideHud::modEnabledHealth{ false };
bool HideHud::modEnabledOrbs{ false };
bool HideHud::modEnabledStyle{ false };
bool HideHud::modEnabledTimer{ false };
bool HideHud::modEnabledBoey{ false };

// DevilMayCry4_DX9.exe+B1997 - cmp byte ptr [esi+04], 01 compares all hud elements, use it while paused to find what you need
// DevilMayCry4_DX9.exe+FEFAC - mov byte ptr [ecx+04], 00 writes dante weapon hud off

// DevilMayCry4_DX9.exe+0FF7AD first 10 hp segments (4 nops)
// DevilMayCry4_DX9.exe+0FF68A first bar of green (4 nops)
// DevilMayCry4_DX9.exe+0FF5A6 first bar of red (4 nops)
// DevilMayCry4_DX9.exe+0FF88C cap of first hp bar (4 nops)
// DevilMayCry4_DX9.exe+0FF9CD last 10 hp segments (4 nops)
// DevilMayCry4_DX9.exe+0FF6DA last bar of green (4 nops)
// DevilMayCry4_DX9.exe+0FF5F6 last bar of red (4 nops)
// DevilMayCry4_DX9.exe+0FFA4C cap on second hp bar (4 nops)
// DevilMayCry4_DX9.exe+0FFF16 first notch of DT (4 nops)
// DevilMayCry4_DX9.exe+0FFF46 second notch of DT (4 nops)
// DevilMayCry4_DX9.exe+0FFF76 third (4 nops)
// DevilMayCry4_DX9.exe+0FFFD0 notch between third and fourth (4 nops)
// DevilMayCry4_DX9.exe+100018 4-10 DT (4 nops)
// DevilMayCry4_DX9.exe+10009C cap on the end of dt (4 nops)
// DevilMayCry4_DX9.exe+1036BB style dial (4 nops)
// DevilMayCry4_DX9.exe+105069 disaster gauge (4 nops)
// DevilMayCry4_DX9.exe+1050A7 disaster cap (4 nops)
// DevilMayCry4_DX9.exe+1050FC disaster fill (4 nops)
// DevilMayCry4_DX9.exe+10383F style letter (4 nops)
// DevilMayCry4_DX9.exe+106E4F dt filled notches (4 nops)
// DevilMayCry4_DX9.exe+10011E dt partly filled notches (4 nops)
// DevilMayCry4_DX9.exe+1051EF rg gauge start (4 nops)
// DevilMayCry4_DX9.exe+105224 rg gauge (4 nops)
// DevilMayCry4_DX9.exe+105269 rg gauge cap (4 nops)
// DevilMayCry4_DX9.exe+1039C1 icon behind style name 01 (4 nops)
// DevilMayCry4_DX9.exe+103B1D icon behind style name 02 (3 nops)
// DevilMayCry4_DX9.exe+103BEA icon behind style name 03 (3 nops)
// DevilMayCry4_DX9.exe+103CE4 icon behind style name 04 (3 nops)
// DevilMayCry4_DX9.exe+103E07 icon behind style name 05 (3 nops)
// DevilMayCry4_DX9.exe+104A43 icon behind style name 06 (3 nops)
// DevilMayCry4_DX9.exe+104B2B icon behind style name 07 (3 nops)
// DevilMayCry4_DX9.exe+104C41 icon behind style name 08 (3 nops)
// DevilMayCry4_DX9.exe+104D43 icon behind style name 09 (3 nops)
// DevilMayCry4_DX9.exe+104DCE icon behind style name 10 (3 nops)
// DevilMayCry4_DX9.exe+104603 icon behind style name 11 (3 nops)
// DevilMayCry4_DX9.exe+1046DA icon behind style name 12 (3 nops)
// DevilMayCry4_DX9.exe+1047B1 icon behind style name 13 (3 nops)
// DevilMayCry4_DX9.exe+1048CF icon behind style name 14 (3 nops)
// DevilMayCry4_DX9.exe+103EB0 icon behind style name 15 (3 nops)
// DevilMayCry4_DX9.exe+104008 icon behind style name 16 (3 nops)
// DevilMayCry4_DX9.exe+104188 icon behind style name 17 (3 nops)
// DevilMayCry4_DX9.exe+10433B icon behind style name 18 (3 nops)
// DevilMayCry4_DX9.exe+10449F icon behind style name 19 (3 nops)
// DevilMayCry4_DX9.exe+103ADB icon behind style name 20 (3 nops)
// DevilMayCry4_DX9.exe+103BA9 icon behind style name 21 (3 nops)
// DevilMayCry4_DX9.exe+103C77 icon behind style name 22 (3 nops)
// DevilMayCry4_DX9.exe+103D8D icon behind style name 23 (3 nops)
// DevilMayCry4_DX9.exe+104941 icon behind style name 24 (3 nops)
// DevilMayCry4_DX9.exe+103F07 icon behind style name 25 (3 nops)
// DevilMayCry4_DX9.exe+104077 icon behind style name 26 (3 nops)
// DevilMayCry4_DX9.exe+10421A icon behind style name 27 (3 nops)
// DevilMayCry4_DX9.exe+1043A5 icon behind style name 28 (3 nops)
// DevilMayCry4_DX9.exe+1044B9 icon behind style name 29 (3 nops)
// DevilMayCry4_DX9.exe+10463B icon behind style name 30 (3 nops)
// DevilMayCry4_DX9.exe+104711 icon behind style name 31 (3 nops)
// DevilMayCry4_DX9.exe+104814 icon behind style name 32 (3 nops)
// DevilMayCry4_DX9.exe+103872 style name 01 (4 nops)
// DevilMayCry4_DX9.exe+10399C style name 02 (4 nops)
// DevilMayCry4_DX9.exe+1049F8 style name 03 (3 nops)
// DevilMayCry4_DX9.exe+104AC0 style name 04 (3 nops)
// DevilMayCry4_DX9.exe+104BB3 style name 05 (3 nops)
// DevilMayCry4_DX9.exe+104CDD style name 06 (3 nops)
// DevilMayCry4_DX9.exe+104DBF style name 07 (3 nops)
// DevilMayCry4_DX9.exe+100519 dt drain 1 (4 nops)
// DevilMayCry4_DX9.exe+1003E1 dt drain 2 (4 nops)
// DevilMayCry4_DX9.exe+1004A8 dt drain 3 (4 nops)
// DevilMayCry4_DX9.exe+1002E6 dt drain 4 (4 nops)
// DevilMayCry4_DX9.exe+10036B dt drain 5 (4 nops)
// DevilMayCry4_DX9.exe+10020B dt drain 6 (4 nops)
// DevilMayCry4_DX9.exe+10519A guard gauge is full (4 nops)
// DevilMayCry4_DX9.exe+108F19 nero dt blobs (4 nops)


std::optional<std::string> HideHud::onInitialize() {
    return Mod::onInitialize();
}

void HideHud::toggleBoey(bool enable) {
    if (enable) {
        install_patch_offset(0x00FF7AD, patchboey01, "\x90\x90\x90\x90", 4); // first 10 hp segments
        install_patch_offset(0x00FF68A, patchboey02, "\x90\x90\x90\x90", 4); // first bar of green
        install_patch_offset(0x00FF5A6, patchboey03, "\x90\x90\x90\x90", 4); // first bar of red
        install_patch_offset(0x00FF88C, patchboey04, "\x90\x90\x90\x90", 4); // cap of first hp bar
        install_patch_offset(0x00FF9CD, patchboey05, "\x90\x90\x90\x90", 4); // last 10 hp segments
        install_patch_offset(0x00FF6DA, patchboey06, "\x90\x90\x90\x90", 4); // last bar of green
        install_patch_offset(0x00FF5F6, patchboey07, "\x90\x90\x90\x90", 4); // last bar of red
        install_patch_offset(0x00FFA4C, patchboey08, "\x90\x90\x90\x90", 4); // cap on second hp bar
        install_patch_offset(0x00FFF16, patchboey09, "\x90\x90\x90\x90", 4); // first notch of dt
        install_patch_offset(0x00FFF46, patchboey10, "\x90\x90\x90\x90", 4); // second notch of dt
        install_patch_offset(0x00FFF76, patchboey11, "\x90\x90\x90\x90", 4); // third notch of dt
        install_patch_offset(0x00FFFD0, patchboey12, "\x90\x90\x90\x90", 4); // notch between third and fourth
        install_patch_offset(0x0100018, patchboey13, "\x90\x90\x90\x90", 4); // 4-10 DT
        install_patch_offset(0x010009C, patchboey14, "\x90\x90\x90\x90", 4); // cap on the end of dt
        // install_patch_offset(0x01036BB, patchboey15, "\x90\x90\x90\x90", 4); // style dial
        install_patch_offset(0x0105069, patchboey16, "\x90\x90\x90\x90", 4); // disaster gauge
        install_patch_offset(0x01050A7, patchboey17, "\x90\x90\x90\x90", 4); // disaster cap
        install_patch_offset(0x01050FC, patchboey18, "\x90\x90\x90\x90", 4); // disaster fill
        // install_patch_offset(0x010383F, patchboey19, "\x90\x90\x90\x90", 4); // style letter
        install_patch_offset(0x0106E4F, patchboey20, "\x90\x90\x90\x90", 4); // dt filled notches
        install_patch_offset(0x010011E, patchboey21, "\x90\x90\x90\x90", 4); // dt partly filled notches
        install_patch_offset(0x01051EF, patchboey22, "\x90\x90\x90\x90", 4); // rg gauge start
        install_patch_offset(0x0105224, patchboey23, "\x90\x90\x90\x90", 4); // rg gauge
        install_patch_offset(0x0105269, patchboey24, "\x90\x90\x90\x90", 4); // rg gauge cap
        install_patch_offset(0x01039C1, patchboey25, "\x90\x90\x90\x90", 4); // icon behind style name 01
        install_patch_offset(0x0103B1D, patchboey26, "\x90\x90\x90",     3); // icon behind style name 02
        install_patch_offset(0x0103BEA, patchboey27, "\x90\x90\x90",     3); // icon behind style name 03
        install_patch_offset(0x0103CE4, patchboey28, "\x90\x90\x90",     3); // icon behind style name 04
        install_patch_offset(0x0103E07, patchboey29, "\x90\x90\x90",     3); // icon behind style name 05
        install_patch_offset(0x0104A43, patchboey30, "\x90\x90\x90",     3); // icon behind style name 06
        install_patch_offset(0x0104B2B, patchboey31, "\x90\x90\x90",     3); // icon behind style name 07
        install_patch_offset(0x0104C41, patchboey32, "\x90\x90\x90",     3); // icon behind style name 08
        install_patch_offset(0x0104D43, patchboey33, "\x90\x90\x90",     3); // icon behind style name 09
        install_patch_offset(0x0104DCE, patchboey34, "\x90\x90\x90",     3); // icon behind style name 10
        install_patch_offset(0x0104603, patchboey35, "\x90\x90\x90",     3); // icon behind style name 11
        install_patch_offset(0x01046DA, patchboey36, "\x90\x90\x90",     3); // icon behind style name 12
        install_patch_offset(0x01047B1, patchboey37, "\x90\x90\x90",     3); // icon behind style name 13
        install_patch_offset(0x01048CF, patchboey38, "\x90\x90\x90",     3); // icon behind style name 14
        install_patch_offset(0x0103EB0, patchboey39, "\x90\x90\x90",     3); // icon behind style name 15
        install_patch_offset(0x0104008, patchboey40, "\x90\x90\x90",     3); // icon behind style name 16
        install_patch_offset(0x0104188, patchboey41, "\x90\x90\x90",     3); // icon behind style name 17
        install_patch_offset(0x010433B, patchboey42, "\x90\x90\x90",     3); // icon behind style name 18
        install_patch_offset(0x010449F, patchboey43, "\x90\x90\x90",     3); // icon behind style name 19
        install_patch_offset(0x0103ADB, patchboey44, "\x90\x90\x90",     3); // icon behind style name 20
        install_patch_offset(0x0103BA9, patchboey45, "\x90\x90\x90",     3); // icon behind style name 21
        install_patch_offset(0x0103C77, patchboey46, "\x90\x90\x90",     3); // icon behind style name 22
        install_patch_offset(0x0103D8D, patchboey47, "\x90\x90\x90",     3); // icon behind style name 23
        install_patch_offset(0x0104941, patchboey48, "\x90\x90\x90",     3); // icon behind style name 24
        install_patch_offset(0x0103F07, patchboey49, "\x90\x90\x90",     3); // icon behind style name 25
        install_patch_offset(0x0104077, patchboey50, "\x90\x90\x90",     3); // icon behind style name 26
        install_patch_offset(0x010421A, patchboey51, "\x90\x90\x90",     3); // icon behind style name 27
        install_patch_offset(0x01043A5, patchboey52, "\x90\x90\x90",     3); // icon behind style name 28
        install_patch_offset(0x01044B9, patchboey53, "\x90\x90\x90",     3); // icon behind style name 29
        install_patch_offset(0x010463B, patchboey54, "\x90\x90\x90",     3); // icon behind style name 30
        install_patch_offset(0x0104711, patchboey55, "\x90\x90\x90",     3); // icon behind style name 31
        install_patch_offset(0x0104814, patchboey56, "\x90\x90\x90",     3); // icon behind style name 32
        install_patch_offset(0x0103872, patchboey57, "\x90\x90\x90\x90", 4); // style name 01
        install_patch_offset(0x010399C, patchboey58, "\x90\x90\x90\x90", 4); // style name 02
        install_patch_offset(0x01049F8, patchboey59, "\x90\x90\x90",     3); // style name 03
        install_patch_offset(0x0104AC0, patchboey60, "\x90\x90\x90",     3); // style name 04
        install_patch_offset(0x0104BB3, patchboey61, "\x90\x90\x90",     3); // style name 05
        install_patch_offset(0x0104CDD, patchboey62, "\x90\x90\x90",     3); // style name 06
        install_patch_offset(0x0104DBF, patchboey63, "\x90\x90\x90",     3); // style name 07
        install_patch_offset(0x0100519, patchboey64, "\x90\x90\x90\x90", 4); // dt drain 01
        install_patch_offset(0x01003E1, patchboey65, "\x90\x90\x90\x90", 4); // dt drain 02
        install_patch_offset(0x01004A8, patchboey66, "\x90\x90\x90\x90", 4); // dt drain 03
        install_patch_offset(0x01002E6, patchboey67, "\x90\x90\x90\x90", 4); // dt drain 04
        install_patch_offset(0x010036B, patchboey68, "\x90\x90\x90\x90", 4); // dt drain 05
        install_patch_offset(0x010020B, patchboey69, "\x90\x90\x90\x90", 4); // dt drain 06
        install_patch_offset(0x010519A, patchboey70, "\x90\x90\x90\x90", 4); // full rg bar
        install_patch_offset(0x0108F19, patchboey71, "\x90\x90\x90\x90", 4); // nero dt blobs
    }
    else {
        patchboey01.revert();
        patchboey02.revert();
        patchboey03.revert();
        patchboey04.revert();
        patchboey05.revert();
        patchboey06.revert();
        patchboey07.revert();
        patchboey08.revert();
        patchboey09.revert();
        patchboey10.revert();
        patchboey11.revert();
        patchboey12.revert();
        patchboey13.revert();
        patchboey14.revert();
        // patchboey15.revert();
        patchboey16.revert();
        patchboey17.revert();
        patchboey18.revert();
        // patchboey19.revert();
        patchboey20.revert();
        patchboey21.revert();
        patchboey22.revert();
        patchboey23.revert();
        patchboey24.revert();
        patchboey25.revert();
        patchboey26.revert();
        patchboey27.revert();
        patchboey28.revert();
        patchboey29.revert();
        patchboey30.revert();
        patchboey31.revert();
        patchboey32.revert();
        patchboey33.revert();
        patchboey34.revert();
        patchboey35.revert();
        patchboey36.revert();
        patchboey37.revert();
        patchboey38.revert();
        patchboey39.revert();
        patchboey40.revert();
        patchboey41.revert();
        patchboey42.revert();
        patchboey43.revert();
        patchboey44.revert();
        patchboey45.revert();
        patchboey46.revert();
        patchboey47.revert();
        patchboey48.revert();
        patchboey49.revert();
        patchboey50.revert();
        patchboey51.revert();
        patchboey52.revert();
        patchboey53.revert();
        patchboey54.revert();
        patchboey55.revert();
        patchboey56.revert();
        patchboey57.revert();
        patchboey58.revert();
        patchboey59.revert();
        patchboey60.revert();
        patchboey61.revert();
        patchboey62.revert();
        patchboey63.revert();
        patchboey64.revert();
        patchboey65.revert();
        patchboey66.revert();
        patchboey67.revert();
        patchboey68.revert();
        patchboey69.revert();
        patchboey70.revert();
        patchboey71.revert();
    }
}

void HideHud::toggleHealth(bool enable) {
    if (enable) {
        install_patch_offset(0x00FEFE5, patchhealth, "\x73\x2E", 2); // Hides health
    }
    else {
        patchhealth.revert();
    }
}
void HideHud::toggleOrbs(bool enable) {
    if (enable){
        install_patch_offset(0x1008A2, patchorbs1,  "\x90\x90\x90",     3); // Hide Orbs icon
        install_patch_offset(0x0FD952, patchorbs2,  "\x90\x90\x90\x90", 4); // Hide Orbs text
        install_patch_offset(0x0FDD80, patchorbs3,  "\x90\x90\x90\x90", 4); // Hide Orbs text
        install_patch_offset(0x0FDC8F, patchorbs4,  "\x90\x90\x90\x90", 4); // Hide Orbs text
        install_patch_offset(0x0FDD16, patchorbs5,  "\x90\x90\x90\x90", 4); // Hide Orbs text
        install_patch_offset(0x100F21, patchorbs6,  "\x90\x90\x90\x90", 4); // Hide Orbs blood
        install_patch_offset(0x10134E, patchorbs8,  "\x90\x90\x90\x90", 4); // Hide Orbs icon outline
        install_patch_offset(0x101240, patchorbs9,  "\x90\x90\x90\x90", 4); // Hide Orbs icon speed lines
        install_patch_offset(0x100B24, patchorbs10, "\xC6\x40\x04\x00", 4); // Hide Orbs icon star // needs to be written 0 very cool
    }
    else {
        patchorbs1.revert();
        patchorbs2.revert();
        patchorbs3.revert();
        patchorbs4.revert();
        patchorbs5.revert();
        patchorbs6.revert();
        patchorbs7.revert();
        patchorbs8.revert();
        patchorbs9.revert();
        patchorbs10.revert();
    }
}
void HideHud::toggleStyle(bool enable) {
    if (enable) {
        install_patch_offset(0x1017F8, patchstyle, "\x0F\x83\x5B\x03\x00\x00", 6); // Hide Style
    }
    else {
        patchstyle.revert();
    }
}
void HideHud::toggleTimer(bool enable) {
    if (enable) {
        install_patch_offset(0x0FDE2A, patchbp, "\x00", 1);
        install_patch_offset(0x0FE088, patchbpplus, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x102DF7, patchbpstageclearbonus1, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x102E04, patchbpstageclearbonus2, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x0FE298, patchm12, "\x00", 1);
        // install_patch_offset(0x50A8E6, patchm12flash, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patchbp.revert();
        patchbpplus.revert();
        patchbpstageclearbonus1.revert();
        patchbpstageclearbonus2.revert();
        patchm12.revert();
        // patchm12flash.revert(); // breaks stage animations such as animated centre of BP or spinning background in BP
    }
}

void HideHud::onGUIframe() {
    if (ImGui::Checkbox("Hide Timer", &modEnabledTimer)) {
        toggleTimer(modEnabledTimer);
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Hide HP HUD", &modEnabledHealth)) {
        toggleHealth(modEnabledHealth);
    }

    if (ImGui::Checkbox("Hide Orbs HUD", &modEnabledOrbs)) {
        toggleOrbs(modEnabledOrbs);
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Hide Style HUD", &modEnabledStyle)) {
        toggleStyle(modEnabledStyle);
    }

    if (ImGui::Checkbox("Style Dial Only", &modEnabledBoey)) {
        toggleBoey(modEnabledBoey);
    }

}

void HideHud::onConfigLoad(const utils::Config& cfg) {
    modEnabledHealth = cfg.get<bool>("hide_hp_hud").value_or(false);
    toggleHealth(modEnabledHealth);
    modEnabledOrbs = cfg.get<bool>("hide_orbs_hud").value_or(false);
    toggleOrbs(modEnabledOrbs);
    modEnabledStyle = cfg.get<bool>("hide_style_hud").value_or(false);
    toggleStyle(modEnabledStyle);
    modEnabledTimer = cfg.get<bool>("hide_timer_hud").value_or(false);
    toggleTimer(modEnabledTimer);
    modEnabledBoey = cfg.get<bool>("hide_boey_hud").value_or(false);
    toggleBoey(modEnabledBoey);
}

void HideHud::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("hide_hp_hud", modEnabledHealth);
    cfg.set<bool>("hide_orbs_hud", modEnabledOrbs);
    cfg.set<bool>("hide_style_hud", modEnabledStyle);
    cfg.set<bool>("hide_timer_hud", modEnabledTimer);
    cfg.set<bool>("hide_boey_hud", modEnabledBoey);
}
