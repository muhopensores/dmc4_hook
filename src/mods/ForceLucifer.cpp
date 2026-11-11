#include "ForceLucifer.hpp"
#include "InputStates.hpp"

#if 1
bool ForceLucifer::mod_enabled = false;
uintptr_t ForceLucifer::force_lucifer_continue = NULL;
bool ForceLucifer::enable_force_ecstasy_timer = false;

static uint8_t rose_flag = 0;

// uPlWpLucifer::setEnable 0x835F9D: esi = the Lucifer, +0x1378 its owner (the Dante it belongs to), al = on / off
naked void force_lucifer_proc(void) {
    _asm {
        push eax
        mov eax, [esi+0x1378]
        test eax, eax
        jz notdante
        cmp byte ptr [eax+0x1494], 0 // controller id in case Nero uses this
    notdante:
        pop eax
        jne code
        cmp dword ptr [esi+0x1378], 0
        je code
        cmp byte ptr [ForceLucifer::mod_enabled], 1
        je ForceLuci

    // check2: the owner's Rose (Taunt Ecstasy) keeps it out
        pushad
        push dword ptr [esi+0x1378]
        call InputStates::rose_active
        mov [rose_flag], al
        popad
        cmp byte ptr [rose_flag], 1
        jne code
    ForceLuci:
        cmp edi, 6 // ID (this accesses gilg etc too)
        jne code
        cmp al,0 // on/off
        je retcode
        jmp code

    code:
        mov [esi+0x0000137C], al
    retcode:
		jmp dword ptr [ForceLucifer::force_lucifer_continue]
    }
}

std::optional<std::string> ForceLucifer::on_initialize() {
    if (!install_hook_offset(0x435F9D, hook, &force_lucifer_proc, &ForceLucifer::force_lucifer_continue, 6)) {
        spdlog::error("Failed to init ForceLucifer mod\n");
        return "Failed to init ForceLucifer mod";
    }

    return Mod::on_initialize();
}

void ForceLucifer::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Force Lucifer"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Forcing Lucifer means never forcefully despawning rose"));
    }
}

void ForceLucifer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("force_lucifer").value_or(false);
};

void ForceLucifer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("force_lucifer", mod_enabled);
};

#endif
