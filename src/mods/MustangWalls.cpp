#include "MustangWalls.hpp"

bool MustangWalls::mod_enabled = false;
uintptr_t MustangWalls::jmp_ret1 = NULL;
uintptr_t MustangWalls::jmp_je = 0x7CA6C7;

// Colliding with a wall kills your momentum, so this is only guardflyable if your spacing is perfect -
// Ideally we use the hitbox to check for walls instead of the push pill.
// 
// This will probably conflict with Air Mustang, but I haven't bothered looking as it's currently MIA
naked void detour1() {
    _asm {
            cmp byte ptr [MustangWalls::mod_enabled], 1
            jne originalcode

            push eax
            mov eax, [esi+0x1E8C] // collision
            test eax, eax
            je originalcode
            cmp byte ptr [eax+0x1F], 1 // touching wall
            pop eax
            je jecode // play mustang

        originalcode:
            cmp byte ptr [esi+0x00014D95], 00
            jmp dword ptr [MustangWalls::jmp_ret1]

        jecode:
            jmp dword ptr [MustangWalls::jmp_je]
    }
}

// void on_frame(fmilliseconds& dt) {}

void MustangWalls::on_gui_frame(int display) {
    ImGui::Checkbox(_("Mustang Walls"), &mod_enabled);
    // ImGui::SameLine();
    // help_marker(_(""));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> MustangWalls::on_initialize() {
    if (!install_hook_offset(0x3CA6BE, hook1, &detour1, &jmp_ret1, 7)) {
        spdlog::error("Failed to init MustangWalls mod 1\n");
        return "Failed to init MustangWalls mod 1";
	}

    return Mod::on_initialize();
}

void MustangWalls::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("MustangWalls").value_or(false);
}

void MustangWalls::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("MustangWalls", mod_enabled);
}
