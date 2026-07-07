#include "ExceedLock.hpp"

bool ExceedLock::mod_enabled = false;
uintptr_t ExceedLock::jmp_ret1 = NULL;
int ExceedLock::exceedLockValue = 0;

naked void detour1() {
    _asm {
            cmp byte ptr [ExceedLock::mod_enabled], 1
            jne originalcode

            mov edi, [ExceedLock::exceedLockValue] // use edi because it gets overwritten anyway
            mov [esi+0x04], edi // exceed value
            inc edi
            mov [esi+0x08], edi // sword effect, refresh checks jnl/jle so we do +1
        originalcode:
            mov edi, [esi+0x04]
            lea edx, [esp+0x05]
            jmp dword ptr [ExceedLock::jmp_ret1]
    }
}

// void ExceedLock::on_frame(fmilliseconds& dt) {}

void ExceedLock::on_gui_frame(int display) {
    if (display == DISPLAY_NERO_A) {
        ImGui::BeginGroup();
        ImGui::Checkbox(_("Exceed Lock"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Lock exceed to any level"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            ImGui::SetNextItemWidth(sameLineItemWidth);
            ImGui::SliderInt("##ExceedLockSliderInt", &exceedLockValue, 0, 3);
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }
}

// void ExceedLock::on_game_pause(bool toggle) {}
// bool ExceedLock::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ExceedLock::on_initialize() {
    if (!install_hook_offset(0x3EA52F, hook1, &detour1, &jmp_ret1, 7)) {
        spdlog::error("Failed to init ExceedLock mod 1\n");
        return "Failed to init ExceedLock mod 1";
	}

    return Mod::on_initialize();
}

void ExceedLock::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ExceedLock").value_or(false);
    exceedLockValue = cfg.get<int>("ExceedLockValue").value_or(1);
}

void ExceedLock::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ExceedLock", mod_enabled);
    cfg.set<int>("ExceedLockValue", exceedLockValue);
}
