#include "EnemyReplace.hpp"

bool EnemyReplace::mod_enabled = false;
static uintptr_t mod_base = (uintptr_t)GetModuleHandle(NULL);
static uintptr_t replacement_address_two = mod_base + 0x24B77B;

static uintptr_t scarecrow_leg_address  = mod_base + 0x13F810; // 0x53F810;
static uintptr_t scarecrow_arm_address  = mod_base + 0x15E710; // 0x55E710;
static uintptr_t scarecrow_mega_address = mod_base + 0x15F7E0; // 0x55F7E0;
static uintptr_t angelo_bianco_address  = mod_base + 0x161A10; // 0x561A10;
static uintptr_t angelo_alto_address    = mod_base + 0x176C80; // 0x576C80;
static uintptr_t mephisto_address       = mod_base + 0x17F1E0; // 0x57F1E0;
static uintptr_t faust_address          = mod_base + 0x195810; // 0x595810;
static uintptr_t frost_address          = mod_base + 0x1A3F60; // 0x5A3F60;
static uintptr_t assault_address        = mod_base + 0x1B3170; // 0x5B3170;
static uintptr_t blitz_address          = mod_base + 0x1D1760; // 0x5D1760;
static uintptr_t chimera_seed_address   = mod_base + 0x1DC160; // 0x5DC160;
static uintptr_t basilisk_address       = mod_base + 0x21A7B0; // 0x61A7B0;
static uintptr_t berial_address         = mod_base + 0x230AC0; // 0x630AC0;
static uintptr_t bael_address           = mod_base + 0x249CB0; // 0x649CB0;
static uintptr_t echidna_address        = mod_base + 0x285340; // 0x685340;
static uintptr_t credo_address          = mod_base + 0x2AA2C0; // 0x6AA2C0;
static uintptr_t agnus_address          = mod_base + 0x2BDE60; // 0x6BDE60;
static uintptr_t sanctus_address        = mod_base + 0x2F81E0; // 0x6F81E0;
static uintptr_t sanctus_dia_address    = mod_base + 0x3022F0; // 0x7022F0;
static uintptr_t kyrie_address          = mod_base + 0x323C00; // 0x723C00;
static uintptr_t dante_address          = mod_base + 0x3BF980; // 0x7BF980;
// modBase = 400000
// crash:                                 
// static uintptr_t CutlassAddress        = mod_base + 0x209C20; // 0x609C20;
// static uintptr_t GladiusAddress        = mod_base + 0x218460; // 0x618460;

// id:         name & notes:                        file spawn bytes
// em000    // Scarecrow (Leg Type)              // B3D5241C
// em001    // Scarecrow (Arm Type)              // 10B4A56C
// em003    // Mega Scarecrow                    // C3D37215
// em005    // Bianco Angelo (White Armor)       // F20A5D0C
// em006    // Alto Angelo (Gold Armor)          // DADC9B79
// em008    // Mephisto                          // 8C051F65
// em009    // Faust                             // 2F7D022E
// em010    // Frost                             // A3218863
// em011    // Assault                           // 3F0CA327
// em012    // Blitz                             // 231B4C1B
// em013    // Chimera                           // E284A500
// em015    // Cutlass                           // 4601B405
// em016    // Gladius                           // 59C0DF2E
// em017    // Basillisk                         // 73B20614
// em018    // Berial                            // 839FE038
// em019    // Bael                              // 1AA82843
// em021    // Echidna                           // 98FCAC35
// em022    // Angelo Credo                      // AD130C4A
// em023    // Angelo Agnus                      // 2AA83A7F
// em025    // Savior                            // 5C0C385E
// em026    // Savior2                           //
// em027    // Credo human form, pl004           //
// em029    // Sanctus, pl002, pl0023            // 62D0876D
// em030    // Sanctus2, pl023                   //
// em031    // Credo human again, pl004          //
// em033    // Savior3                           //
// em035    // style checker, em027, orb005      //
// em036    // Kyrie, pl022                      //
// em_dante // Boss Dante                        // 3D355100

std::optional<std::string> EnemyReplace::on_initialize() {
    return Mod::on_initialize();
}

naked void scarecrow_leg_naked(void) {
	_asm { jmp dword ptr [scarecrow_leg_address] }
}
naked void scarecrow_arm_naked(void) {
    _asm { jmp dword ptr [scarecrow_arm_address] }
}
naked void mega_naked(void) {
    _asm { jmp dword ptr [scarecrow_mega_address] }
}
naked void bianco_naked(void) {
    _asm { jmp dword ptr [angelo_bianco_address] }
}
naked void alto_naked(void) {
    _asm { jmp dword ptr [angelo_alto_address] }
}
naked void mephisto_naked(void) {
    _asm { jmp dword ptr [mephisto_address] }
}
naked void faust_naked(void) {
    _asm { jmp dword ptr [faust_address] }
}
naked void frost_naked(void) {
    _asm { jmp dword ptr [frost_address] }
}
naked void assault_naked(void) {
    _asm { jmp dword ptr [assault_address] }
}
naked void blitz_naked(void) {
    _asm { jmp dword ptr [blitz_address] }
}
naked void chimera_seed_naked(void) {
    _asm { jmp dword ptr [chimera_seed_address] }
}
naked void basilisk_naked(void) {
    _asm { jmp dword ptr [basilisk_address] }
}
naked void berial_naked(void) {
    _asm { jmp dword ptr [berial_address] }
}
naked void bael_naked(void) {
    _asm { jmp dword ptr [bael_address] }
}
naked void echidna_naked(void) {
    _asm { jmp dword ptr [echidna_address] }
}
naked void credo_naked(void) {
    _asm { jmp dword ptr [credo_address] }
}
naked void agnus_naked(void) {
    _asm { jmp dword ptr [agnus_address] }
}
naked void sanctus_naked(void) {
    _asm { jmp dword ptr [sanctus_address] }
}
naked void sanctus_dia_naked(void)
{
    _asm { jmp dword ptr [sanctus_dia_address] }
}
naked void kyrie_naked(void) {
    _asm { jmp dword ptr [kyrie_address] }
}
naked void dante_naked(void) {
    _asm { jmp dword ptr [dante_address] }
}
/*naked void CutlassNaked(void) { // broken
    _asm { jmp dword ptr [CutlassAddress] }
}
naked void GladiusNaked(void) { // broken
    _asm { jmp dword ptr [GladiusAddress] }
}*/

const char* get_bytes_to_patch(int desired_enemy_id) {
    switch (desired_enemy_id) {
        case  0: return (char*)&scarecrow_leg_naked;
        case  1: return (char*)&scarecrow_arm_naked;
        case  2: return (char*)&mega_naked;
        case  3: return (char*)&bianco_naked;
        case  4: return (char*)&alto_naked;
        case  5: return (char*)&mephisto_naked;
        case  6: return (char*)&faust_naked;
        case  7: return (char*)&frost_naked;
        case  8: return (char*)&assault_naked;
        case  9: return (char*)&blitz_naked;
        case 10: return (char*)&chimera_seed_naked;
        case 11: return (char*)&basilisk_naked;
        case 12: return (char*)&berial_naked;
        case 13: return (char*)&bael_naked;
        case 14: return (char*)&echidna_naked;
        case 15: return (char*)&credo_naked;
        case 16: return (char*)&agnus_naked;
        case 17: return (char*)&sanctus_naked;
        case 18: return (char*)&sanctus_dia_naked;
        case 19: return (char*)&kyrie_naked;
        case 20: return (char*)&dante_naked;
        default: __assume(false); // should not be reachable
    }
}
const char* replacement_address_two74 = "\x74"; // default, written when 73 isn't needed
const char* replacement_address_two73 = "\x73";

std::unique_ptr<Patch>& EnemyReplace::get_patch(int enemyID) {
    switch (enemyID) {
        case  0: return replace_scarecrow_leg_patch;
        case  1: return replace_scarecrow_arm_patch;
        case  2: return replace_scarecrow_mega_patch;
        case  3: return replace_angelo_bianco_patch;
        case  4: return replace_angelo_alto_patch;
        case  5: return replace_mephisto_patch;
        case  6: return replace_faust_patch;
        case  7: return replace_frost_patch;
        case  8: return replace_assault_patch;
        case  9: return replace_blitz_patch;
        case 10: return replace_chimera_patch;
        case 11: return replace_basilisk_patch;
        case 12: return replace_berial_patch;
        case 13: return replace_bael_patch;
        case 14: return replace_echidna_patch;
        case 15: return replace_credo_patch;
        case 16: return replace_agnus_patch;
        case 17: return replace_sanctus_patch;
        case 18: return replace_sanctus_dia_patch;
        case 19: return replace_kyrie_patch;
        case 20: return replace_dante_patch;
        default: __assume(false); // should not be reachable
    }
}

uintptr_t EnemyReplace::get_enemy_address(int enemy_id) {
    switch (enemy_id) {
        case  0: return scarecrow_leg_address;
        case  1: return scarecrow_arm_address;
        case  2: return scarecrow_mega_address;
        case  3: return angelo_bianco_address;
        case  4: return angelo_alto_address;
        case  5: return mephisto_address;
        case  6: return faust_address;
        case  7: return frost_address;
        case  8: return assault_address;
        case  9: return blitz_address;
        case 10: return chimera_seed_address;
        case 11: return basilisk_address;
        case 12: return berial_address;
        case 13: return bael_address;
        case 14: return echidna_address;
        case 15: return credo_address;
        case 16: return agnus_address;
        case 17: return sanctus_address;
        case 18: return sanctus_dia_address;
        case 19: return kyrie_address;
        case 20: return dante_address;
        default: __assume(false); // should not be reachable
    }
}

void EnemyReplace::replace_enemy_with(int current_enemy_id, int desired_enemy_id) {
    // disable jmping to self
    if (current_enemy_id == desired_enemy_id) {
        get_patch(current_enemy_id).reset();
    }
    else {
        install_patch_absolute(get_enemy_address(current_enemy_id), get_patch(current_enemy_id), get_bytes_to_patch(desired_enemy_id), 6);
        //GetPatch(currentEnemyID)->apply(GetEnemyAddress(currentEnemyID), GetBytesToPatch(desiredEnemyID), 6);
    }

    if (desired_enemy_id == 13 && current_enemy_id != 13) { // bael requires an extra byte set (but don't set it if already bael)
        install_patch_absolute(replacement_address_two, replacement_address_two_patch, replacement_address_two73, 1);
        //replacementAddressTwo_patch.apply(replacementAddressTwo, ReplacementAddressTwo73, 1);
    }
    else { // default
        install_patch_absolute(replacement_address_two, replacement_address_two_patch, replacement_address_two74, 1);
    }
}

const char* combo_lists[] = {
    __("Replace Scarecrow (Leg) with:"),    // 00
    __("Replace Scarecrow (Arm) with:"),    // 01
    __("Replace Mega Scarecrow with:"),     // 02
    __("Replace Bianco Angelo with:"),      // 03
    __("Replace Alto Angelo with:"),        // 04
    __("Replace Mephisto with:"),           // 05
    __("Replace Faust with:"),              // 06
    __("Replace Frost with:"),              // 07
    __("Replace Assault with:"),            // 08
    __("Replace Blitz with:"),              // 09
    __("Replace Chimera Seed with:"),       // 10
    __("Replace Basilisk with:"),           // 11
    __("Replace Berial with:"),             // 12
    __("Replace Bael with:"),               // 13
    __("Replace Echidna with:"),            // 14
    __("Replace Credo with:"),              // 15
    __("Replace Agnus with:"),              // 16
    __("Replace Sanctus with:"),            // 17
    __("Replace Sanctus Diabolica with:"),  // 18
    __("Replace Kyrie with:"),              // 19
    __("Replace Dante with:")               // 20
};

const char* combo_items[] = {
    __("Scarecrow (Leg)"),    // 00
    __("Scarecrow (Arm)"),    // 01
    __("Mega Scarecrow"),     // 02
    __("Bianco Angelo"),      // 03
    __("Alto Angelo"),        // 04
    __("Mephisto"),           // 05
    __("Faust"),              // 06
    __("Frost"),              // 07
    __("Assault"),            // 08
    __("Blitz"),              // 09
    __("Chimera Seed"),       // 10
    __("Basilisk"),           // 11
    __("Berial"),             // 12
    __("Bael"),               // 13
    __("Echidna"),            // 14
    __("Credo"),              // 15
    __("Agnus"),              // 16
    __("Sanctus"),            // 17
    __("Sanctus Diabolica"),  // 18
    __("Kyrie"),              // 19
    __("Dante")               // 20
};

// combo list text doesn't seem to like format text
const char* combo_list_i_ds[] = {
    "##ReplaceScarecrowLeg",  // 00
    "##ReplaceScarecrowArm",  // 01
    "##ReplaceMegaScarecrow", // 02
    "##ReplaceBiancoAngelo",  // 03
    "##ReplaceAltoAngelo",    // 04
    "##ReplaceMephisto",      // 05
    "##ReplaceFaust",         // 06
    "##ReplaceFrost",         // 07
    "##ReplaceAssault",       // 08
    "##ReplaceBlitz",         // 09
    "##ReplaceChimeraSeed",   // 10
    "##ReplaceBasilisk",      // 11
    "##ReplaceBerial",        // 12
    "##ReplaceBael",          // 13
    "##ReplaceEchidna",       // 14
    "##ReplaceCredo",         // 15
    "##ReplaceAgnus",         // 16
    "##ReplaceSanctus",       // 17
    "##ReplaceDiaSanctus",    // 18
    "##ReplaceKyrie",         // 19
    "##ReplaceDante"          // 20
};

// neither does save/loading
const char* save_load_i_ds[] = {
    "ReplaceScarecrowLeg",  // 00
    "ReplaceScarecrowArm",  // 01
    "ReplaceMegaScarecrow", // 02
    "ReplaceBiancoAngelo",  // 03
    "ReplaceAltoAngelo",    // 04
    "ReplaceMephisto",      // 05
    "ReplaceFaust",         // 06
    "ReplaceFrost",         // 07
    "ReplaceAssault",       // 08
    "ReplaceBlitz",         // 09
    "ReplaceChimeraSeed",   // 10
    "ReplaceBasilisk",      // 11
    "ReplaceBerial",        // 12
    "ReplaceBael",          // 13
    "ReplaceEchidna",       // 14
    "ReplaceCredo",         // 15
    "ReplaceAgnus",         // 16
    "ReplaceSanctus",       // 17
    "ReplaceDiaSanctus",    // 18
    "ReplaceKyrie",         // 19
    "ReplaceDante"          // 20
};

// TODO Map??????
int EnemyReplace::default_enemy[IM_ARRAYSIZE(combo_lists)] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
int EnemyReplace::desired_enemy[IM_ARRAYSIZE(combo_items)];

void EnemyReplace::on_gui_frame(int display) {
    ImGui::PushItemWidth(sameLineItemWidth);
    utility::ImGooListboxTranslated localizedText(combo_lists, IM_ARRAYSIZE(combo_lists));
    utility::ImGooListboxTranslated localizedBoxes(combo_items, IM_ARRAYSIZE(combo_items));
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        ImGui::Text(localizedText.data_[i]);
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Combo(combo_list_i_ds[i], &desired_enemy[i], localizedBoxes.data_, IM_ARRAYSIZE(combo_items))) {
            replace_enemy_with(default_enemy[i], desired_enemy[i]);
        }
    }
    ImGui::PopItemWidth();
    ImGui::Text(_("Reset Enemy Replacements:"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Button(_("Reset"))) {
        for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
            desired_enemy[i] = i;
            replace_enemy_with(default_enemy[i], desired_enemy[i]);
        }
    }
}

void EnemyReplace::on_config_load(const utility::Config& cfg) {
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        desired_enemy[i] = cfg.get<int>("enemy_replace_id_"+std::to_string(i)).value_or(i);
        replace_enemy_with(default_enemy[i], desired_enemy[i]);
    }
}

void EnemyReplace::on_config_save(utility::Config& cfg) {
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        cfg.set<int>("enemy_replace_id_"+std::to_string(i), desired_enemy[i]);
    }
}
