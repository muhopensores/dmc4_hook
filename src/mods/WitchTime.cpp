#include "WitchTime.hpp"
#include "Quicksilver.hpp"

bool WitchTime::witch_time_enabled_dante = false;
bool WitchTime::witch_time_enabled_nero = false;
uintptr_t WitchTime::jmp_ret1 = NULL;
constexpr uintptr_t static_mediator_ptr  = 0x00E558B8; // we really should put this somewhere global
static constexpr float wt_duration = 3.0f;

/*void WitchTime::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x0, patch1, "\x90", 1);
    }
    else {
        patch1.reset();
    }
}*/

naked void detour1() {
    _asm {
        cmp byte ptr [WitchTime::witch_time_enabled_dante], 1
        je checkdante
        cmp byte ptr [WitchTime::witch_time_enabled_nero], 1
        je checknero
        jmp originalcode

        checkdante:
        cmp dword ptr [esp+0xC], 5 // dodge
        jne originalcode
        push eax
        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp byte ptr [eax+0x1494], 0 // controller id
        je witchcode
        pop eax
        cmp byte ptr [WitchTime::witch_time_enabled_nero], 1
        je checknero
        jmp originalcode

        checknero:
        cmp dword ptr [esp+0xC], 5 // dodge
        jne originalcode
        push eax
        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp byte ptr [eax+0x1494], 1 // controller id
        je witchcode
        jmp popcode

        witchcode:
        cmp byte ptr [eax+0x2998], 4 // moveid - jump
        je popcode
        cmp byte ptr [eax+0x2998], 9 // moveid - jump back
        je popcode
        cmp byte ptr [eax+0x2998], 5 // moveid - jump forward
        je popcode
        pop eax
        pushad
        push dword ptr [wt_duration]
        call Quicksilver::start_quicksilver
        add esp, 4
        popad
        jmp originalcode

        popcode:
        pop eax
        originalcode:
        cvttss2si ecx, [esp+0x14]
        jmp dword ptr [WitchTime::jmp_ret1]
    }
}

// void WitchTime::on_frame(fmilliseconds& dt) {}

void WitchTime::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Witch Time##Dante"), &witch_time_enabled_dante);
    }
    if (display == DISPLAY_NERO_A) {
        ImGui::Checkbox(_("Witch Time##Nero"), &witch_time_enabled_nero);
    }
    if (display == DISPLAY_DANTE_A || display == DISPLAY_NERO_A) {
        ImGui::SameLine();
        help_marker(_("Slow down the world when you dodge an attack with anything other than jump"));
    }
}

// void WitchTime::on_game_pause(bool toggle) {}
// bool WitchTime::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> WitchTime::on_initialize() {
    if (!install_hook_offset(0xA6023, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init WitchTime mod 1\n");
        return "Failed to init WitchTime mod 1";
	}

    return Mod::on_initialize();
}

void WitchTime::on_config_load(const utility::Config& cfg){
    witch_time_enabled_dante = cfg.get<bool>("witch_time_enabled_dante").value_or(false);
    witch_time_enabled_nero = cfg.get<bool>("witch_time_enabled_nero").value_or(false);
}

void WitchTime::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("witch_time_enabled_dante", witch_time_enabled_dante);
    cfg.set<bool>("witch_time_enabled_nero", witch_time_enabled_nero);
}
