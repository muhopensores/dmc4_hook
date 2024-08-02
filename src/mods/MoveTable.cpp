#include "MoveTable.hpp"
#include "misc/kAtckDefTbl.cpp"

bool MoveTable::mod_enabled { false };

uintptr_t  MoveTable::jmp_ret1 { NULL };
uintptr_t  MoveTable::jmp_ret2 { NULL };
uintptr_t  MoveTable::jmp_ret3 { NULL };

constexpr uintptr_t NativeDanteKADTbl = 0x00C3FEA0;
constexpr uintptr_t NativeNeroKADTbl = 0x00C3EE40;
uintptr_t HookDanteKADTbl { NULL };
uintptr_t HookNeroKADTbl { NULL };


struct kAtckDefTbl {
    uint32_t atckAttr;
    uint32_t atckId;
    uint32_t atckLevel;
    uint32_t atckInfo;
    union command{
        uint32_t commandBits;
        struct {
            uint8_t atckCommand;
            uint8_t atckCommandNo;
            uint8_t atckCondition;
            uint8_t ukn;
        };
    };
    uint32_t atckConditionWp;
    uint32_t atckConditionStyle;
    uint32_t atckAs;
    uint32_t cancelId[5];
};

void __stdcall move_switch(uint32_t moveID, uintptr_t actor) {
    switch (moveID + 1) {
    case 0x6D: {
        //Add stuffs here
        uintptr_t splash_func = 0x007D6ED0;
        (((void (__stdcall *)(uintptr_t))splash_func)(actor));
        break;
    }
    default:
        return;
    }
    return;
}

void MoveTable::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x403C6B, patch1, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch1.reset();
    }
}

naked void detour1(void) { //Assign Dante's kAtckDefTbl
    _asm  {
            push eax
            cmp byte ptr [MoveTable::mod_enabled],1
            jne handler
            mov eax,HookDanteKADTbl
            mov dword ptr [edi+0x1DCC],eax
        originalcode:
            pop eax
            jmp [MoveTable::jmp_ret1]
        handler:
            mov eax,[NativeDanteKADTbl]
            mov [edi+0x1DCC],eax
            jmp originalcode
    }
}

naked void detour2(void) { //Assign Nero's kAtckDefTbl
    _asm  {
            push eax
            cmp byte ptr [MoveTable::mod_enabled],1
            jne handler
            mov eax,HookNeroKADTbl
            mov dword ptr [esi+0x1DCC],eax
        originalcode:
            pop eax
            jmp [MoveTable::jmp_ret2]
        handler:
            mov eax,[NativeNeroKADTbl]
            mov [esi+0x1DCC],eax
            jmp originalcode
    }
}

naked void detour3(void) { //handle Dante's move call
    _asm {
            jna originalcode
            pushad
            push eax //uPlayerDante
            push ecx //moveID
            call move_switch
            popad
            pop edi
            pop esi
            mov esp,ebp
            pop ebp
            ret
        originalcode:
            jmp [MoveTable::jmp_ret3]
    }
}

void updateKDATbl() {
    //Put stuffs here
    DanteAtckDefTbl.insert(DanteAtckDefTbl.begin(), {2, 0x6D, 7, 1, 3, 6, (unsigned long)-1, 0, 2, 1, 0, 0, 0, 0x05000007}); //New splash
    //Terminate
    DanteAtckDefTbl.emplace_back(3);
    NeroAtckDefTbl.emplace_back(3);
    HookDanteKADTbl = (uintptr_t)DanteAtckDefTbl.data();
    HookNeroKADTbl  = (uintptr_t)NeroAtckDefTbl.data();
}

std::optional<std::string> MoveTable::on_initialize() {
    updateKDATbl();
    if (!install_hook_offset(0x3B21E5, hook1, &detour1, &jmp_ret1, 10)) {
		spdlog::error("Failed to init MoveTable mod\n");
		return "Failed to init MoveTable mod";
	}
    if (!install_hook_offset(0x3E1CAC, hook2, &detour2, &jmp_ret2, 10)) {
		spdlog::error("Failed to init MoveTable mod2\n");
		return "Failed to init MoveTable mod2";
	}
    if (!install_hook_offset(0x3CD1A6, hook3, &detour3, &jmp_ret3, 6)) {
		spdlog::error("Failed to init MoveTable mod2\n");
		return "Failed to init MoveTable mod2";
	}

    return Mod::on_initialize();
}

void MoveTable::on_gui_frame() {
    if (ImGui::Checkbox(_("Move Table"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Replace internal move params"));
}

void MoveTable::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("move_table").value_or(false);
    toggle(mod_enabled);
};

void MoveTable::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("move_table", mod_enabled);
};