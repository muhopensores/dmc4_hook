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
    // Put stuff here, IDs must be > 0x6B (107)
    DanteAtckDefTbl.insert(DanteAtckDefTbl.begin(), {2, 0x6D, 7, 1, 3, 6, (unsigned long)-1, 0, 2, 1, 0, 0, 0, 0x05000007}); // New splash

    // Terminate
    DanteAtckDefTbl.emplace_back(3);
    NeroAtckDefTbl.emplace_back(3);
    HookDanteKADTbl = (uintptr_t)DanteAtckDefTbl.data();
    HookNeroKADTbl  = (uintptr_t)NeroAtckDefTbl.data();
}

#if 0
uint32_t CompareArrays(const std::vector<unsigned int>& arr1,const std::vector<unsigned int>& arr2) {
    if (arr1.size() != arr2.size()) {
        return -1;
    }
    for (size_t i = 0; i < arr1.size(); ++i) {
        if (arr1[i] != arr2[i]) {
            return i;
        }
    }
    return 0x6969;
}
#endif

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

void MoveTable::on_frame(fmilliseconds& dt) {
    if (!display_move_table) { return; }
    uintptr_t player = (uintptr_t)devil4_sdk::get_local_player();
    if (player) {
        ImVec2 screenRes = devil4_sdk::get_sRender()->screenRes;
        ImGui::SetNextWindowPos(ImVec2(sameLineWidth * 2.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(screenRes.x - sameLineWidth * 2.0f, 0.0f));
        if (ImGui::Begin("Attack Definitions", &display_move_table)) {
            float tableStart = ImGui::GetCursorPosY();
            static ImGuiTableFlags flags = 
                ImGuiTableFlags_RowBg | 
                ImGuiTableFlags_Borders | 
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_NoPadOuterX;
        
            if (ImGui::BeginTable("##AtckDefTable", 18, flags, ImVec2(0.0f, screenRes.y * 0.9f - tableStart))) {
                ImGui::TableSetupScrollFreeze(0, 1);
                
                ImGui::TableSetupColumn("Entry");
                ImGui::TableSetupColumn("Attr");
                ImGui::TableSetupColumn("Id");
                ImGui::TableSetupColumn("Level");
                ImGui::TableSetupColumn("Info");
                ImGui::TableSetupColumn("Cmd");
                ImGui::TableSetupColumn("CmdNo");
                ImGui::TableSetupColumn("Cnd");
                ImGui::TableSetupColumn("ukn");
                ImGui::TableSetupColumn("CndWp");
                ImGui::TableSetupColumn("CndStyle");
                ImGui::TableSetupColumn("aerial");
                ImGui::TableSetupColumn("atckAs");
                ImGui::TableSetupColumn("cancel0");
                ImGui::TableSetupColumn("cancel1");
                ImGui::TableSetupColumn("cancel2");
                ImGui::TableSetupColumn("cancel3");
                ImGui::TableSetupColumn("cancel4");
                ImGui::TableHeadersRow();
                
                uintptr_t kAtckDefTblPtr = (uintptr_t)*(uintptr_t*)(player + 0x1DCC);
                kAtckDefTbl* CountTblEntry = (kAtckDefTbl*)(kAtckDefTblPtr);

                const size_t valuesPerEntry = 14;
                uint32_t totalEntries = 0;
                while ((CountTblEntry->atckAttr != 3)) {
                    totalEntries++;
                    CountTblEntry++;
                }
                
                for (size_t entryIdx = 0; entryIdx < totalEntries; entryIdx++) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); 
                    ImGui::Text("%zu", entryIdx);
                    
                    const size_t baseIdx = entryIdx * valuesPerEntry;
                    kAtckDefTbl* TblEntry = (kAtckDefTbl*)(kAtckDefTblPtr + (entryIdx * sizeof(kAtckDefTbl)));
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckAttr);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckId);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckLevel);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckInfo);
                    ImGui::TableNextColumn(); ImGui::Text("%02X", TblEntry->command.atckCommand);
                    ImGui::TableNextColumn(); ImGui::Text("%02X", TblEntry->command.atckCommandNo);
                    ImGui::TableNextColumn(); ImGui::Text("%02X", TblEntry->command.atckCondition);
                    ImGui::TableNextColumn(); ImGui::Text("%02X", TblEntry->command.ukn);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckConditionWp);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckConditionStyle);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->ukn);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->atckAs);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->cancelId[0]);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->cancelId[1]);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->cancelId[2]);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->cancelId[3]);
                    ImGui::TableNextColumn(); ImGui::Text("%08X", TblEntry->cancelId[4]);
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }
}

void MoveTable::display_attack_entry(kAtckDefTbl* TblEntry) {
    static int step = 1;
    ImGui::PushID((uintptr_t)TblEntry);
    ImGui::InputScalar(_("+0 Attribute"), ImGuiDataType_U32, &TblEntry->atckAttr, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+4 Attack ID"), ImGuiDataType_U32, &TblEntry->atckId, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+8 Level"), ImGuiDataType_U32, &TblEntry->atckLevel, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+C Info"), ImGuiDataType_U32, &TblEntry->atckInfo, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("Enables Cmd Settings?"));
    ImGui::InputScalar(_("+10 Cmd"), ImGuiDataType_U8, &TblEntry->command.atckCommand, &step, 0, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("20 = on gun release, 21 = on gun charge release"));
    ImGui::InputScalar(_("+11 Cmd No"), ImGuiDataType_U8, &TblEntry->command.atckCommandNo, &step, 0, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+12 Cmd Cond"), ImGuiDataType_U8, &TblEntry->command.atckCondition, &step, 0, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+13 Cmd Ukn"), ImGuiDataType_U8, &TblEntry->command.ukn, &step, 0, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+14 Weapon Condition"), ImGuiDataType_U32, &TblEntry->atckConditionWp, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+18 Style Condition"), ImGuiDataType_U32, &TblEntry->atckConditionStyle, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+1C Aerial"), ImGuiDataType_U32, &TblEntry->ukn, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+20 atckAs"), ImGuiDataType_U32, &TblEntry->atckAs, &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+24 Cancel 0"), ImGuiDataType_U32, &TblEntry->cancelId[0], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+28 Cancel 1"), ImGuiDataType_U32, &TblEntry->cancelId[1], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+2C Cancel 2"), ImGuiDataType_U32, &TblEntry->cancelId[2], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+30 Cancel 3"), ImGuiDataType_U32, &TblEntry->cancelId[3], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+34 Cancel 4"), ImGuiDataType_U32, &TblEntry->cancelId[4], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopID();
}

void MoveTable::on_gui_frame(int display) {
    ImGui::Checkbox(_("Display Move Table"), &display_move_table);
    uPlayer* player = devil4_sdk::get_local_player();
    
    if (player) {
        ImGui::Indent(lineIndent);
        if (ImGui::CollapsingHeader(_("Display Table Editor"))) {
            uintptr_t kAtckDefTblPtr = (uintptr_t&)player->kAtckDefTblPtr;
            kAtckDefTbl* CountTblEntry = (kAtckDefTbl*)(kAtckDefTblPtr);
            const size_t valuesPerEntry = 14;
            int totalEntries = 0;
            while ((CountTblEntry->atckAttr != 3)) {
                totalEntries++;
                CountTblEntry++;
            }
            static int selectedEntryIndex = 0;
            if (selectedEntryIndex >= totalEntries)
                selectedEntryIndex = totalEntries - 1;
            if (selectedEntryIndex < 0)
                selectedEntryIndex = 0;
            kAtckDefTbl* TblEntry = (kAtckDefTbl*)(kAtckDefTblPtr + (selectedEntryIndex * sizeof(kAtckDefTbl)));
            uint32_t currentMoveId = player->moveID2;
            ImGui::InputScalar(_("Player Attack ID"), ImGuiDataType_U32, &currentMoveId, 0, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
            int EntryCount = 0;
            if (ImGui::Button(_("Find Current Attack ID"))) {
                int entryNum = 0;
                kAtckDefTbl* entry = (kAtckDefTbl*)kAtckDefTblPtr;
                while (entryNum < totalEntries && (entry->atckId != currentMoveId) && (entry->atckAttr != 3)) {
                    entryNum++;
                    entry++;
                }
                if (entryNum < totalEntries) {
                    selectedEntryIndex = entryNum;
                }
            }
            ImGui::InputInt(_("Entry Num"), &selectedEntryIndex, 1, 10);
            ImGui::Text(_("Entry count: %i"), totalEntries);
            display_attack_entry(TblEntry);
            ImGui::Indent(lineIndent);
            if (ImGui::CollapsingHeader(_("Create a new entry"))) {
                static kAtckDefTbl customTblEntry{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
                if (ImGui::Button(_("Fill fields from currently displayed entry"))) {
                    customTblEntry.atckAttr = TblEntry->atckAttr;
                    customTblEntry.atckId = TblEntry->atckId;
                    customTblEntry.atckLevel = TblEntry->atckLevel;
                    customTblEntry.atckInfo = TblEntry->atckInfo;
                    customTblEntry.command.buffer = TblEntry->command.buffer;
                    customTblEntry.atckConditionWp = TblEntry->atckConditionWp;
                    customTblEntry.atckConditionStyle = TblEntry->atckConditionStyle;
                    customTblEntry.ukn = TblEntry->ukn;
                    customTblEntry.atckAs = TblEntry->atckAs;
                    customTblEntry.cancelId[0] = TblEntry->cancelId[0];
                    customTblEntry.cancelId[1] = TblEntry->cancelId[1];
                    customTblEntry.cancelId[2] = TblEntry->cancelId[2];
                    customTblEntry.cancelId[3] = TblEntry->cancelId[3];
                    customTblEntry.cancelId[4] = TblEntry->cancelId[4];
                }
                display_attack_entry(&customTblEntry);
                if (ImGui::Button(_("Create"))) {
                    DanteAtckDefTbl.insert(DanteAtckDefTbl.begin(), {
                        customTblEntry.atckAttr,
                        customTblEntry.atckId,
                        customTblEntry.atckLevel,
                        customTblEntry.atckInfo,
                        customTblEntry.command.buffer,
                        customTblEntry.atckConditionWp,
                        customTblEntry.atckConditionStyle,
                        customTblEntry.ukn,
                        customTblEntry.atckAs,
                        customTblEntry.cancelId[0],
                        customTblEntry.cancelId[1],
                        customTblEntry.cancelId[2],
                        customTblEntry.cancelId[3],
                        customTblEntry.cancelId[4]
                        });
                    if (player->controllerID == 0) {
                        player->kAtckDefTblPtr = (kAtckDefTbl*)HookDanteKADTbl;
                        DanteAtckDefTbl.emplace_back(3);
                    }
                    else {
                        player->kAtckDefTblPtr = (kAtckDefTbl*)HookNeroKADTbl;
                        NeroAtckDefTbl.emplace_back(3);
                    }
                }
            }
            TblEntry = (kAtckDefTbl*)(uintptr_t)(kAtckDefTblPtr);
            ImGui::Unindent(lineIndent);
        }
        ImGui::Unindent(lineIndent);
    }
    else {
        ImGui::Indent(lineIndent);
        ImGui::Text(_("Load into a stage to see the table and the editor"));
        ImGui::Unindent(lineIndent);
    }
}

void MoveTable::on_config_load(const utility::Config& cfg) {
	// mod_enabled = cfg.get<bool>("move_table").value_or(false);
    // toggle(mod_enabled);
};

void MoveTable::on_config_save(utility::Config& cfg) {
	// cfg.set<bool>("move_table", mod_enabled);
};
