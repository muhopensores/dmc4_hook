#include "MustangWalls.hpp"

bool MustangWalls::mod_enabled = false;
uintptr_t MustangWalls::jmp_ret1 = NULL;
uintptr_t MustangWalls::jmp_je = 0x7CA6C7;

uintptr_t MustangWalls::jmp_ret2 = NULL;
static constexpr float maxMustangFrame = 25.0f;

// Colliding with a wall kills your momentum, so this is only guardflyable if your spacing is perfect -
// Ideally we use the hitbox to check for walls instead of the push pill.
// 
// This will probably conflict with Air Mustang, but I haven't bothered looking as it's currently MIA

naked void detour1() { // trickster dash
    _asm {
            cmp byte ptr [MustangWalls::mod_enabled], 1
            jne originalcode

            test byte ptr [esi+0x140D], 2 // input held
            je originalcode
            movss xmm6, [esi+0x1E8C] // current frame
            comiss xmm6, [maxMustangFrame]
            ja originalcode

            push eax
            mov eax, [esi+0x1E8C] // collision
            test eax, eax
            je originalcodepop
            cmp byte ptr [eax+0x1F], 1 // touching wall
            pop eax
            je jecode // play mustang
            jmp originalcode

        originalcodepop:
            pop eax
        originalcode:
            cmp byte ptr [esi+0x00014D95], 00
            jmp dword ptr [MustangWalls::jmp_ret1]

        jecode:
            jmp dword ptr [MustangWalls::jmp_je]
    }
}

naked void detour2() { // sky star
    _asm {
            cmp byte ptr [MustangWalls::mod_enabled], 1
            jne originalcode

            test byte ptr [esi+0x140D], 2 // input held
            je originalcode
            movss xmm6, [esi+0x348] // current frame
            comiss xmm6, [maxMustangFrame]
            ja originalcode

            push eax
            mov eax, [esi+0x1E8C] // collision
            test eax, eax
            je codepop
            cmp byte ptr [eax+0x1F], 1 // touching wall
            pop eax
            jne originalcode
            jmp redirect

        redirect:
            push eax
            mov eax, [esi]
            mov edx, [eax+0x194]
            pop eax
            push 0x29
            mov ecx,esi
            call edx
            jmp originalcode

        codepop:
            pop eax
        originalcode:
            pop edi
            pop esi
            mov esp,ebp
            pop ebp
            jmp dword ptr [MustangWalls::jmp_ret2]
    }
}

// void on_frame(fmilliseconds& dt) {}

void MustangWalls::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Mustang Walls"), &mod_enabled);
        // ImGui::SameLine();
        // help_marker(_(""));
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> MustangWalls::on_initialize() {
    if (!install_hook_offset(0x3CA6BE, hook1, &detour1, &jmp_ret1, 7)) {
        spdlog::error("Failed to init MustangWalls mod 1\n");
        return "Failed to init MustangWalls mod 1";
	}

    if (!install_hook_offset(0x3CAE77, hook2, &detour2, &jmp_ret2, 5)) {
		spdlog::error("Failed to init MustangWalls mod 2\n");
		return "Failed to init MustangWalls mod 2";
	}

    return Mod::on_initialize();
}

void MustangWalls::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("MustangWalls").value_or(false);
}

void MustangWalls::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("MustangWalls", mod_enabled);
}
