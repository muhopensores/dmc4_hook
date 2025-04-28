#include "MoveTable.hpp"

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

// When we need to access values from this array, add this to keep it consistent
int MoveTable::extra_nero_moves = 0;
int MoveTable::extra_dante_moves = 0;

constexpr uintptr_t NativeDanteKADTbl = 0x00C3FEA0;
constexpr uintptr_t NativeNeroKADTbl = 0x00C3EE40;
uintptr_t HookDanteKADTbl { NULL };
uintptr_t HookNeroKADTbl { NULL };
static bool display_move_table { false };

int MoveTable::AirThrow = 0;

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
            // cmp byte ptr [AerialStinger::mod_enabled], 1
            // je newcode
            // cmp byte ptr [LuciAirThrow::mod_enabled], 1
            // je newcode
            // jmp originalcode

        // newcode:
            mov eax, HookDanteKADTbl
            mov dword ptr [edi+0x1DCC], eax
            jmp cont

        // originalcode:
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
            // cmp byte ptr [Payline::mod_enabled], 1
            // je newcode
            // jmp originalcode

        // newcode:
            mov eax, HookNeroKADTbl
            mov dword ptr [esi+0x1DCC], eax
            jmp cont

        // originalcode:
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

// Moves on the front have priority so its necessary to put them at the start
// Initialize with buffer.commandNo set to 1 (5th value, 6th digit) so it's impossible to use until the cheat tickbox or load sets it 0
void updateKDATbl() {
    // Put stuff here, IDs must be > 0x6B (107)
    
    // DanteAtckDefTbl.insert(DanteAtckDefTbl.begin(), {}); // 
    // MoveTable::Entry = MoveTable::extra_dante_moves;
    // MoveTable::extra_dante_moves++;

    DanteAtckDefTbl.insert(DanteAtckDefTbl.begin(), {2, 0x6D, 7, 1, 0x00000103, 6, (unsigned long)-1, 0, 2, 1, 0, 0, 0, 0x05000007}); // New splash
    MoveTable::extra_dante_moves++; // keep track of original table / newly added elements
    MoveTable::AirThrow = MoveTable::extra_dante_moves; // Store index so we can find it

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
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        ImVec2 screenRes = devil4_sdk::get_sRender()->screenRes;
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(screenRes.x * 1.0f, screenRes.y * 0.4f), ImGuiCond_Once);
        if (ImGui::Begin("Attack Definitions UI", &display_move_table)) {
            static ImGuiTableFlags flags =
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_ScrollX;
            ImGui::Text(_("Player Attack ID = %i"), player->moveID2);
            ImGui::SameLine();
            if (ImGui::Button("Use Current Move ID")) {
                sprintf_s(columnSearchBuffers[2], "%d", player->moveID2);
                columnSearchActive[2] = true;
                columnSearchIsExact[2] = enableExactMatch;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear All Filters")) {
                for (int i = 0; i < 18; i++) {
                    columnSearchBuffers[i][0] = '\0';
                    columnSearchActive[i] = false;
                }
            }
            ImGui::SameLine();
            bool oldExactMatch = enableExactMatch;
            if (ImGui::Checkbox("Exact Match", &enableExactMatch)) {
                if (oldExactMatch != enableExactMatch) {
                    for (int i = 0; i < 18; i++) {
                        if (columnSearchActive[i]) {
                            columnSearchIsExact[i] = enableExactMatch;
                        }
                    }
                }
            }
            ImGui::SameLine();
            ImGui::Text(_("Nero Moves Added = %i"), MoveTable::extra_nero_moves);
            ImGui::SameLine();
            ImGui::Text(_("Dante Moves Added = %i"), MoveTable::extra_dante_moves);

            const char* columnHeaders[18] = {
                "Entry", "Attr", "Id", "Level", "CmdTgl", "Cmd", "CmdNo", "Cnd", "Ukn", 
                "CndWp", "CndStyle", "CndDT", "CndAir", 
                "cancel0", "cancel1", "cancel2", "cancel3", "cancel4"
            };
            
            float fontSize = ImGui::GetFontSize();
            float fontMultipliers[18] = {
                2.0f,  // Entry
                2.0f,  // Attr
                2.0f,  // Id
                2.0f,  // Level
                3.0f,  // CmdTgl
                15.0f, // Cmd
                2.0f,  // CmdNo
                2.0f,  // Cnd
                2.0f,  // Ukn
                8.0f,  // CndWp
                8.5f,  // CndStyle
                8.5f,  // CndDT
                8.5f,  // CndAir
                5.0f,  // cancel0
                5.0f,  // cancel1
                5.0f,  // cancel2
                5.0f,  // cancel3
                5.0f   // cancel4
            };
            
            float columnWidths[18];
            for (int i = 0; i < 18; i++) {
                columnWidths[i] = fontSize * fontMultipliers[i];
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            if (ImGui::BeginTable("##AtckDefTable", 18, flags)) {
                ImGui::TableSetupScrollFreeze(0, 2);
                
                for (int i = 0; i < 18; i++) {
                    ImGui::TableSetupColumn(columnHeaders[i], ImGuiTableColumnFlags_WidthFixed, columnWidths[i]);
                }
                
                ImGui::TableHeadersRow();
                
                ImGui::TableNextRow();
                for (int i = 0; i < 18; i++) {
                    ImGui::TableNextColumn();
                    
                    ImGui::SetNextItemWidth(columnWidths[i] - ImGui::GetStyle().ItemInnerSpacing.x);
                    
                    if (ImGui::InputText(("##search_" + std::to_string(i)).c_str(), columnSearchBuffers[i], COLUMN_SEARCH_BUFFER_SIZE)) {
                        bool wasActive = columnSearchActive[i];
                        columnSearchActive[i] = strlen(columnSearchBuffers[i]) > 0;
                        if (!wasActive && columnSearchActive[i]) {
                            columnSearchIsExact[i] = enableExactMatch;
                        }
                    }
                }
                
                uintptr_t kAtckDefTblPtr = (uintptr_t)player->kAtckDefTblPtr;
                kAtckDefTbl* CountTblEntry = (kAtckDefTbl*)(kAtckDefTblPtr);

                const size_t valuesPerEntry = 14;
                uint32_t totalEntries = 0;
                while ((CountTblEntry->atckAttr != 3)) {
                    totalEntries++;
                    CountTblEntry++;
                }
                
                for (size_t entryIdx = 0; entryIdx < totalEntries; entryIdx++) {
                    const size_t baseIdx = entryIdx * valuesPerEntry;
                    kAtckDefTbl* TblEntry = (kAtckDefTbl*)(kAtckDefTblPtr + (entryIdx * sizeof(kAtckDefTbl)));
                    bool showEntry = true;
                    auto matchesSearch = [](const std::string& strValue, const char* searchText, bool isExact) -> bool {
                        std::string search = searchText;
                        if (search.empty()) return true;
                        if (isExact) {
                            return strValue == search;
                        }
                        return strValue.find(search) != std::string::npos;
                    };
                    
                    auto matchesNumericSearch = [&matchesSearch](int value, const char* searchText, bool isExact) -> bool {
                        std::string decStr = std::to_string(value);
                        char hexBuf[16];
                        snprintf(hexBuf, sizeof(hexBuf), "%x", value);
                        std::string hexStr = hexBuf;
                        snprintf(hexBuf, sizeof(hexBuf), "%X", value);
                        std::string hexStrUpper = hexBuf;
                        if (value == -1) {
                            hexStr = "ff";
                            hexStrUpper = "FF";
                        }
                        return matchesSearch(decStr, searchText, isExact) || 
                               matchesSearch(hexStr, searchText, isExact) || 
                               matchesSearch(hexStrUpper, searchText, isExact);
                    };
                    
                    if (columnSearchActive[0] && !matchesNumericSearch(entryIdx, columnSearchBuffers[0], columnSearchIsExact[0])) {
                        showEntry = false;
                    }
                    if (showEntry && columnSearchActive[1] && !matchesNumericSearch(TblEntry->atckAttr, columnSearchBuffers[1], columnSearchIsExact[1]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[2] && !matchesNumericSearch(TblEntry->atckId, columnSearchBuffers[2], columnSearchIsExact[2]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[3] && !matchesNumericSearch(TblEntry->atckLevel, columnSearchBuffers[3], columnSearchIsExact[3]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[4] && !matchesNumericSearch(TblEntry->atckInfo, columnSearchBuffers[4], columnSearchIsExact[4]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[5] && !matchesNumericSearch(TblEntry->command.atckCommand, columnSearchBuffers[5], columnSearchIsExact[5]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[6] && !matchesNumericSearch(TblEntry->command.atckCommandNo, columnSearchBuffers[6], columnSearchIsExact[6]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[7] && !matchesNumericSearch(TblEntry->command.atckCondition, columnSearchBuffers[7], columnSearchIsExact[7]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[8] && !matchesNumericSearch(TblEntry->command.ukn, columnSearchBuffers[8], columnSearchIsExact[8]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[9] && !matchesNumericSearch(TblEntry->atckConditionWp, columnSearchBuffers[9], columnSearchIsExact[9]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[10] && !matchesNumericSearch(TblEntry->atckConditionStyle, columnSearchBuffers[10], columnSearchIsExact[10]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[11] && !matchesNumericSearch(TblEntry->ukn, columnSearchBuffers[11], columnSearchIsExact[11]))
                        showEntry = false;
                    if (showEntry && columnSearchActive[12] && !matchesNumericSearch(TblEntry->atckAs, columnSearchBuffers[12], columnSearchIsExact[12]))
                        showEntry = false;
                    
                    for (int i = 0; i < 5; i++) {
                        int searchIdx = 13 + i;
                        if (showEntry && columnSearchActive[searchIdx]) {
                            std::string hexStr = formatHex(TblEntry->cancelId[i]);
                            
                            if (!matchesSearch(hexStr, columnSearchBuffers[searchIdx], columnSearchIsExact[searchIdx])) {
                                showEntry = false;
                                break;
                            }
                        }
                    }
                    if (!showEntry) {
                        continue;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); 
                    ImGui::Text("%zu", entryIdx);
                    ImGui::PushID((uintptr_t)TblEntry);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[1] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputInt(_("##+0 Attribute"), (int*)&TblEntry->atckAttr, 0, 0);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[2] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputInt(_("##+4 Attack ID"), (int*)&TblEntry->atckId, 0, 0);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[3] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputInt(_("##+8 Attack Level"), (int*)&TblEntry->atckLevel, 0, 0);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[4] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::Combo(_("##+C Enable Command Settings"), (int*)&TblEntry->atckInfo, "Off\0On\0");
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[5] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    int tempCommand = (int)TblEntry->command.atckCommand;
                    if (ImGui::Combo(_("##+10"), &tempCommand, buttonMappingNames, IM_ARRAYSIZE(buttonMappingNames))) {
                        TblEntry->command.atckCommand = (uint8_t)tempCommand;
                    }

                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[6] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar("##+11 cmd no", ImGuiDataType_U8, &TblEntry->command.atckCommandNo);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[7] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar("##+12 cond", ImGuiDataType_U8, &TblEntry->command.atckCondition);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[8] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar("##+13 ukn", ImGuiDataType_U8, &TblEntry->command.ukn);
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[9] - ImGui::GetStyle().ItemInnerSpacing.x);
                    int tempWeapon = TblEntry->atckConditionWp + 1;
                    if (ImGui::Combo(_("##+14 Weapon Condition"), &tempWeapon, weaponNames, IM_ARRAYSIZE(weaponNames))) {
                        TblEntry->atckConditionWp = tempWeapon - 1;
                    }

                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[10] - ImGui::GetStyle().ItemInnerSpacing.x);
                    int tempStyle = TblEntry->atckConditionStyle + 1;
                    if (ImGui::Combo(_("##+18 Style Condition"), &tempStyle, styleNames, IM_ARRAYSIZE(styleNames))) {
                        TblEntry->atckConditionStyle = tempStyle - 1;
                    }

                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[11] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::Combo(_("##+1C DT Condition"), (int*)&TblEntry->ukn, dtNames, IM_ARRAYSIZE(dtNames));
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[12] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::Combo(_("##+20 Aerial Condition"), (int*)&TblEntry->atckAs, aerialNames, IM_ARRAYSIZE(aerialNames));
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[13] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar(_("##+24"), ImGuiDataType_U32, &TblEntry->cancelId[0], 0, 0, "%08x");
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[14] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar(_("##+28"), ImGuiDataType_U32, &TblEntry->cancelId[1], 0, 0, "%08x");
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[15] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar(_("##+2C"), ImGuiDataType_U32, &TblEntry->cancelId[2], 0, 0, "%08x");
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[16] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar(_("##+30"), ImGuiDataType_U32, &TblEntry->cancelId[3], 0, 0, "%08x");
                    
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(columnWidths[17] - ImGui::GetStyle().ItemInnerSpacing.x); 
                    ImGui::InputScalar(_("##+34"), ImGuiDataType_U32, &TblEntry->cancelId[4], 0, 0, "%08x");
                    
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
            ImGui::End();
        }
    }
}

void MoveTable::display_attack_entry(kAtckDefTbl* TblEntry) {
    static int step = 1;
    ImGui::PushID((uintptr_t)TblEntry);
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::InputInt(_("+0 Attribute"), (int*)&TblEntry->atckAttr, 1);
    ImGui::InputInt(_("+4 Attack ID"), (int*)&TblEntry->atckId, 1, 10);
    ImGui::InputInt(_("+8 Level"), (int*)&TblEntry->atckLevel, 1, 10);
    ImGui::Combo(_("+C Enable Command Settings"), (int*)&TblEntry->atckInfo, "Off\0On\0");
    ImGui::Indent(lineIndent);
    int tempCommand = (int)TblEntry->command.atckCommand;
    ImGui::SetNextItemWidth(sameLineItemWidth * 2);
    if (ImGui::Combo(_("+10 Cmd"), &tempCommand, buttonMappingNames, IM_ARRAYSIZE(buttonMappingNames))) {
        TblEntry->command.atckCommand = (uint8_t)tempCommand;
    }
    ImGui::InputScalar(_("+11 Cmd No"), ImGuiDataType_U8, &TblEntry->command.atckCommandNo, &step);
    ImGui::InputScalar(_("+12 Cmd Cond"), ImGuiDataType_U8, &TblEntry->command.atckCondition, &step);
    ImGui::InputScalar(_("+13 Cmd Ukn"), ImGuiDataType_U8, &TblEntry->command.ukn, &step);
    ImGui::Unindent(lineIndent);
    int tempWeapon = TblEntry->atckConditionWp + 1;
    if (ImGui::Combo(_("+14 Weapon Condition"), &tempWeapon, weaponNames, IM_ARRAYSIZE(weaponNames))) {
        TblEntry->atckConditionWp = tempWeapon - 1;
    }
    int tempStyle = TblEntry->atckConditionStyle + 1;
    if (ImGui::Combo(_("+18 Style Condition"), &tempStyle, styleNames, IM_ARRAYSIZE(styleNames))) {
        TblEntry->atckConditionStyle = tempStyle - 1;
    }
    ImGui::SetNextItemWidth(sameLineItemWidth * 2);
    ImGui::Combo(_("+1C DT Condition"), (int*)&TblEntry->ukn, dtNames, IM_ARRAYSIZE(dtNames));
    ImGui::Combo(_("+20 Aerial Condition"), (int*)&TblEntry->atckAs, aerialNames, IM_ARRAYSIZE(aerialNames));
    ImGui::InputScalar(_("+24 Cancel 0"), ImGuiDataType_U32, &TblEntry->cancelId[0], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+28 Cancel 1"), ImGuiDataType_U32, &TblEntry->cancelId[1], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+2C Cancel 2"), ImGuiDataType_U32, &TblEntry->cancelId[2], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+30 Cancel 3"), ImGuiDataType_U32, &TblEntry->cancelId[3], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputScalar(_("+34 Cancel 4"), ImGuiDataType_U32, &TblEntry->cancelId[4], &step, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();
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
            ImGui::InputInt(_("Player Attack ID"), (int*)&currentMoveId);
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
                    if (player->controllerID == 0) {
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
                        player->kAtckDefTblPtr = (kAtckDefTbl*)HookDanteKADTbl;
                        extra_dante_moves++;
                    }
                    else {
                    NeroAtckDefTbl.insert(NeroAtckDefTbl.begin(), {
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
                        player->kAtckDefTblPtr = (kAtckDefTbl*)HookNeroKADTbl;
                        extra_nero_moves++;
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
