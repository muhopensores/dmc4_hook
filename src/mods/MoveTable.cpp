#include "MoveTable.hpp"
#include "misc/kAtckDefTbl.cpp"

// mods that require this:
#include "AerialStinger.hpp"
#include "Payline.hpp"
#include "LuciAirThrow.hpp"
// WARNING: you must also compare to these bools in LMT slot fix

// bool MoveTable::mod_enabled { false };

uintptr_t  MoveTable::jmp_ret0 { NULL };
uintptr_t jmp_je0 = 0x803D37;
uintptr_t  MoveTable::jmp_ret1 { NULL };
uintptr_t  MoveTable::jmp_ret2 { NULL };
uintptr_t  MoveTable::jmp_ret3 { NULL };

constexpr uintptr_t NativeDanteKADTbl = 0x00C3FEA0;
constexpr uintptr_t NativeNeroKADTbl = 0x00C3EE40;
uintptr_t HookDanteKADTbl { NULL };
uintptr_t HookNeroKADTbl { NULL };
static bool display_move_table { false };

//struct kAtckDefTbl {
//    uint32_t atckAttr;
//    uint32_t atckId;
//    uint32_t atckLevel;
//    uint32_t atckInfo;
//    union {
//        uint32_t buffer;
//        struct {
//            uint8_t atckCommand;
//            uint8_t atckCommandNo;
//            uint8_t atckCondition;
//            uint8_t ukn;
//        };
//    } command;
//    uint32_t atckConditionWp;
//    uint32_t atckConditionStyle;
//    uint32_t ukn; //aerial lock
//    uint32_t atckAs;
//    uint32_t cancelId[5];
//};

/*void MoveTable::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x403C6B, patch1, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch1.reset();
    }
}*/

naked void detour0(void) { // MoveTable toggle() function
    _asm  {
            cmp byte ptr [AerialStinger::mod_enabled], 1
            je retcode
            cmp byte ptr [Payline::mod_enabled], 1
            je retcode
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je retcode
        // originalcode:
            test [ecx+eax*4+0x18], edx
            je jecode
        retcode:
            jmp dword ptr [MoveTable::jmp_ret0]
        jecode:
            jmp dword ptr [jmp_je0]
    }
}

naked void detour1(void) { // Assign Dante's kAtckDefTbl
    _asm  {
            push eax
            cmp byte ptr [AerialStinger::mod_enabled], 1
            je newcode
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            mov eax, HookDanteKADTbl
            mov dword ptr [edi+0x1DCC], eax
            jmp cont

        originalcode:
            mov eax, [NativeDanteKADTbl]
            mov [edi+0x1DCC], eax
        cont:
            pop eax
            jmp [MoveTable::jmp_ret1]
    }
}

naked void detour2(void) { // Assign Nero's kAtckDefTbl
    _asm  {
            push eax
            cmp byte ptr [Payline::mod_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            mov eax, HookNeroKADTbl
            mov dword ptr [esi+0x1DCC], eax
            jmp cont

        originalcode:
            mov eax, [NativeNeroKADTbl]
            mov [esi+0x1DCC], eax
        cont:
            pop eax
            jmp [MoveTable::jmp_ret2]
    }
}

// Handle Dante's new >0x6B move ids
void __stdcall dante_move_switch(uint32_t moveID, uintptr_t actor) {
    switch (moveID + 1) {
        case 0x6D: {
            uintptr_t splash_func = 0x007D6ED0;
            (((void(__stdcall*)(uintptr_t))splash_func)(actor));
            break;
        }
        default:
            return;
    }
}

// uintptr_t detour3_ja = 0x7CD504;
naked void detour3(void) { // Handle Dante's move call
    _asm { 
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je newcode

            cmp ecx, 0x6B
            push esi
            push edi
            ja jacode
        retcode:
            jmp dword ptr [MoveTable::jmp_ret3]

        newcode:
            cmp ecx, 0x6B
            push esi
            push edi
            jna retcode // old moveid, continue
            pushad
            push eax // uPlayerDante
            push ecx // moveID
            call dante_move_switch
            popad
        jacode:
            pop edi
            pop esi
            mov esp,ebp
            pop ebp
            ret
    }
}

void updateKDATbl() {
    // Put stuff here, IDs must be > 0x6B
    DanteAtckDefTbl.insert(DanteAtckDefTbl.begin(), {2, 0x6D, 7, 1, 3, 6, (unsigned long)-1, 0, 2, 1, 0, 0, 0, 0x05000007}); //New splash

    // Terminate
    DanteAtckDefTbl.emplace_back(3);
    NeroAtckDefTbl.emplace_back(3);
    HookDanteKADTbl = (uintptr_t)DanteAtckDefTbl.data();
    HookNeroKADTbl  = (uintptr_t)NeroAtckDefTbl.data();
}

std::optional<std::string> MoveTable::on_initialize() {
    updateKDATbl();
    if (!install_hook_offset(0x403C67, hook0, &detour0, &jmp_ret0, 10)) { // MoveTable toggle() function
		spdlog::error("Failed to init MoveTable mod 0\n");
		return "Failed to init MoveTable mod 0";
	}
    if (!install_hook_offset(0x3B21E5, hook1, &detour1, &jmp_ret1, 10)) {
		spdlog::error("Failed to init MoveTable mod 1\n");
		return "Failed to init MoveTable mod 1";
	}
    if (!install_hook_offset(0x3E1CAC, hook2, &detour2, &jmp_ret2, 10)) {
		spdlog::error("Failed to init MoveTable mod2\n");
		return "Failed to init MoveTable mod 2";
	}
    if (!install_hook_offset(0x3CD1A1, hook3, &detour3, &jmp_ret3, 11)) {
		spdlog::error("Failed to init MoveTable mod3\n");
		return "Failed to init MoveTable mod 3";
	}

    return Mod::on_initialize();
}

void MoveTable::on_gui_frame() {
    /*
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Move Table"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Replace internal move params\nRequired by \"Aerial Stinger\", \"Payline\" and \"Lucifer Air Throw\""));
#ifndef NDEBUG
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("[DEBUG] Display move table"), &display_move_table);
        if (display_move_table) {
            uintptr_t player = (uintptr_t)devil4_sdk::get_local_player();
            if (player) {
                ImGui::Indent(lineIndent);
                uintptr_t kAtckDefTblPtr = (uintptr_t) * (uintptr_t*)(player + 0x1DCC);
                uint32_t currentMoveId = (uint32_t) * (uintptr_t*)(player + 0x1564);
                ImGui::InputScalar(_("Player move ID"), ImGuiDataType_U32, &currentMoveId);
                kAtckDefTbl* TblEntry = (kAtckDefTbl*)(kAtckDefTblPtr);
                int EntryCount = 0;
                while ((currentMoveId != TblEntry->atckId) && (TblEntry->atckAttr != 3)) {
                    EntryCount++;
                    TblEntry++;
                }
                ImGui::InputScalar(_("Entry count"), ImGuiDataType_U32, &EntryCount, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
                ImGui::InputScalar(_("Move Attribute"), ImGuiDataType_U32, &TblEntry->atckAttr);
                ImGui::InputScalar(_("Table move ID"), ImGuiDataType_U32, &TblEntry->atckId);
                ImGui::InputScalar(_("Move Level"), ImGuiDataType_U32, &TblEntry->atckLevel);
                ImGui::InputScalar(_("Move Info"), ImGuiDataType_U32, &TblEntry->atckInfo);
                ImGui::InputScalar(_("Move Command"), ImGuiDataType_U32, &TblEntry->command.buffer);
                ImGui::InputScalar(_("Move Weapon Condition"), ImGuiDataType_U32, &TblEntry->atckConditionWp);
                ImGui::InputScalar(_("Move Style Condition"), ImGuiDataType_U32, &TblEntry->atckConditionStyle);
                ImGui::InputScalar(_("Move Attribute"), ImGuiDataType_U32, &TblEntry->atckConditionStyle);
                ImGui::InputScalar(_("Unknown Param"), ImGuiDataType_U32, &TblEntry->ukn);
                ImGui::InputScalar(_("Move Status"), ImGuiDataType_U32, &TblEntry->atckAs);
                ImGui::InputScalar(_("Move Cancel ID 1"), ImGuiDataType_U32, &TblEntry->cancelId[0]);
                ImGui::InputScalar(_("Move Cancel ID 2"), ImGuiDataType_U32, &TblEntry->cancelId[1]);
                ImGui::InputScalar(_("Move Cancel ID 3"), ImGuiDataType_U32, &TblEntry->cancelId[2]);
                ImGui::InputScalar(_("Move Cancel ID 4"), ImGuiDataType_U32, &TblEntry->cancelId[3]);
                ImGui::InputScalar(_("Move Cancel ID 5"), ImGuiDataType_U32, &TblEntry->cancelId[4]);
                TblEntry = (kAtckDefTbl*)(uintptr_t)(kAtckDefTblPtr);
                ImGui::Unindent(lineIndent);
            }
            else {
                ImGui::Indent(lineIndent);
                ImGui::Text(_("Load into a stage to see this table"));
                ImGui::Unindent(lineIndent);
            }
        }
        ImGui::Unindent(lineIndent);
    }
#endif
    ImGui::EndGroup();
    */
}

void MoveTable::on_config_load(const utility::Config& cfg) {
	// mod_enabled = cfg.get<bool>("move_table").value_or(false);
    // toggle(mod_enabled);
};

void MoveTable::on_config_save(utility::Config& cfg) {
	// cfg.set<bool>("move_table", mod_enabled);
};
