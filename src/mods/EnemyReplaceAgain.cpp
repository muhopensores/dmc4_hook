#include "EnemyReplaceAgain.hpp"

bool EnemyReplaceAgain::mod_enabled{ false };
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
// static uintptr_t CutlassAddress        = modBase + 0x209C20; // 0x609C20;
// static uintptr_t GladiusAddress        = modBase + 0x218460; // 0x618460;

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

std::optional<std::string> EnemyReplaceAgain::on_initialize() {
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

std::unique_ptr<Patch>& EnemyReplaceAgain::get_patch(int enemyID) {
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

uintptr_t EnemyReplaceAgain::get_enemy_address(int enemy_id) {
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

void EnemyReplaceAgain::replace_enemy_with(int current_enemy_id, int desired_enemy_id) {
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
    "Replace Scarecrow (Leg) with:",    // 00
    "Replace Scarecrow (Arm) with:",    // 01
    "Replace Mega Scarecrow with:",     // 02
    "Replace Bianco Angelo with:",      // 03
    "Replace Alto Angelo with:",        // 04
    "Replace Mephisto with:",           // 05
    "Replace Faust with:",              // 06
    "Replace Frost with:",              // 07
    "Replace Assault with:",            // 08
    "Replace Blitz with:",              // 09
    "Replace Chimera Seed with:",       // 10
    "Replace Basilisk with:",           // 11
    "Replace Berial with:",             // 12
    "Replace Bael with:",               // 13
    "Replace Echidna with:",            // 14
    "Replace Credo with:",              // 15
    "Replace Agnus with:",              // 16
    "Replace Sanctus with:",            // 17
    "Replace Sanctus Diabolica with:",  // 18
    "Replace Kyrie with:",              // 19
    "Replace Dante with:"               // 20
};

const char* combo_items[] = {
    "Scarecrow (Leg)",    // 00
    "Scarecrow (Arm)",    // 01
    "Mega Scarecrow",     // 02
    "Bianco Angelo",      // 03
    "Alto Angelo",        // 04
    "Mephisto",           // 05
    "Faust",              // 06
    "Frost",              // 07
    "Assault",            // 08
    "Blitz",              // 09
    "Chimera Seed",       // 10
    "Basilisk",           // 11
    "Berial",             // 12
    "Bael",               // 13
    "Echidna",            // 14
    "Credo",              // 15
    "Agnus",              // 16
    "Sanctus",            // 17
    "Sanctus Diabolica",  // 18
    "Kyrie",              // 19
    "Dante"               // 20
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
int EnemyReplaceAgain::default_enemy[IM_ARRAYSIZE(combo_lists)] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
int EnemyReplaceAgain::desired_enemy[IM_ARRAYSIZE(combo_items)];

void EnemyReplaceAgain::on_gui_frame() {
    ImGui::Text("Enemy Replacement");
    ImGui::SameLine();
    help_marker(_("Replace almost any enemy type by clicking the original enemy followed by the desired swapped enemy in the "
                "dropdown\nDo not replace Mephisto with Faust and Faust with Mephisto at the same time. The game will crash"));
    ImGui::Spacing();
    if (ImGui::Button(_("Reset"))) {
        for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
            desired_enemy[i] = i;
            replace_enemy_with(default_enemy[i], desired_enemy[i]);
        }
    }
    ImGui::Spacing();
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        ImGui::Text(combo_lists[i]);
        if (ImGui::Combo(combo_list_i_ds[i], &desired_enemy[i], combo_items, IM_ARRAYSIZE(combo_items))) {
            replace_enemy_with(default_enemy[i], desired_enemy[i]);
        }
        ImGui::Spacing();
    }
}
// TODO cfg??
/* ??:
enemy_replace_id_0=0
enemy_replace_id_1=1
enemy_replace_id_10=10
?????
???????????????
*/
void EnemyReplaceAgain::on_config_load(const utility::Config& cfg) {
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        desired_enemy[i] = cfg.get<int>("enemy_replace_id_"+std::to_string(i)).value_or(i);
        replace_enemy_with(default_enemy[i], desired_enemy[i]);
    }
}

void EnemyReplaceAgain::on_config_save(utility::Config& cfg) {
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        cfg.set<int>("enemy_replace_id_"+std::to_string(i), desired_enemy[i]);
    }
}

// scarecrow leg 00t / 00c
/*
00 Default                  0x53f810=8b0d4c43e1
01 Scarecrow(Arm)           0x53f810=e9fbee0100
02 Mega Scarecrow           0x53f810=e9cbff0100
03 Bianco Angelo            0x53f810=e9fb210200
04 Alto Angelo              0x53f810=e96b740300
05 Mephisto                 0x53f810=e9cbf90300
06 Faust                    0x53f810=e9fb5f0500
07 Frost                    0x53f810=e94b470600
08 Assault                  0x53f810=e95b390700
09 Blitz                    0x53f810=e94b1f0900
10 Chimera Seed             0x53f810=e94bc90900
11 Basilisk                 0x53f810=e99baf0d00
12 Berial                   0x53f810=e9ab120f00
13 Angelo Credo             0x53f810=e9abaa1600
14 Abgelo Agnus             0x53f810=e94be61700
15 Sanctus                  0x53f810=e9db2a1c00
16 Dante                    0x53f810=e96b012800
*/

// scarecrow arm 01t / 01c
/*
00 Default                  0x55e710=8b0d4c43e1
01 Scarecrow(Leg)           0x55e710=e9fb10feff
02 Mega Scarecrow           0x55e710=e9cb100000
03 Bianco Angelo            0x55e710=e9fb320000
04 Alto Angelo              0x55e710=e96b850100
05 Mephisto                 0x55e710=e9cb0a0200
06 Faust                    0x55e710=e9fb700300
07 Frost                    0x55e710=e94b580400
08 Assault                  0x55e710=e95b4a0500
09 Blitz                    0x55e710=e94b300700
10 Chimera Seed             0x55e710=e94bda0700
11 Basilisk                 0x55e710=e99bc00b00
12 Berial                   0x55e710=e9ab230d00
13 Angelo Credo             0x55e710=e9abbb1400
14 Abgelo Agnus             0x55e710=e94bf71500
15 Sanctus                  0x55e710=e9db3b1a00
16 Dante                    0x55e710=e96b122600
*/

// scarecrow mega 03t / 03c
/*
00 Default                  0x55f7e0=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           0x55f7e0=e92b00feff
02 Scarecrow(Arm)           0x55f7e0=e92befffff
03 Bianco Angelo            0x55f7e0=e92b220000
04 Alto Angelo              0x55f7e0=e99b740100
05 Mephisto                 0x55f7e0=e9fbf90100
06 Faust                    0x55f7e0=e92b600300
07 Frost                    0x55f7e0=e97b470400
08 Assault                  0x55f7e0=e98b390500
09 Blitz                    0x55f7e0=e97b1f0700
10 Chimera Seed             0x55f7e0=e97bc90700
11 Basilisk                 0x55f7e0=e9cbaf0b00
12 Berial                   0x55f7e0=e9db120d00
13 Bael                     0x55f7e0=e9cba40e00, 0x64b77b=73
14 Echidna                  0x55f7e0=e95b5b1200
15 Angelo Credo             0x55f7e0=e9dbaa1400
16 Abgelo Agnus             0x55f7e0=e97be61500
17 Sanctus                  0x55f7e0=e90b2b1a00
18 Kyrie                    0x55f7e0=e91b441c00
19 Dante                    0x55f7e0=e99b012600
*/

// bianco 05t / 05c
/*
00 Default                  0x561a10=8b0d4c43e1
01 Scarecrow(Leg)           0x561a10=e9fbddfdff
02 Scarecrow(Arm)           0x561a10=e9fbccffff
03 Mega Scarecrow           0x561a10=e9cbddffff
04 Alto Angelo              0x561a10=e96b520100
05 Mephisto                 0x561a10=e9cbd70100
06 Faust                    0x561a10=e9fb3d0300
07 Frost                    0x561a10=e94b250400
08 Assault                  0x561a10=e95b170500
09 Blitz                    0x561a10=e94bfd0600
10 Chimera Seed             0x561a10=e94ba70700
11 Basilisk                 0x561a10=e99b8d0b00
12 Berial                   0x561a10=e9abf00c00
13 Angelo Credo             0x561a10=e9ab881400
14 Abgelo Agnus             0x561a10=e94bc41500
15 Sanctus                  0x561a10=e9db081a00
16 Dante                    0x561a10=e96bdf2500
*/

// alto 06t / 06c
/*
00 Default                  0x576c80=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           0x576c80=e98b8bfcff
02 Scarecrow(Arm)           0x576c80=e98b7afeff
03 Mega Scarecrow           0x576c80=e95b8bfeff
04 Bianco Angelo            0x576c80=e98badfeff
05 Mephisto                 0x576c80=e95b850000
06 Faust                    0x576c80=e98beb0100
07 Frost                    0x576c80=e9dbd20200
08 Assault                  0x576c80=e9ebc40300
09 Blitz                    0x576c80=e9dbaa0500
10 Chimera Seed             0x576c80=e9db540600
11 Basilisk                 0x576c80=e92b3b0a00
12 Berial                   0x576c80=e93b9e0b00
13 Bael                     0x576c80=e92b300d00, 0x64b77b=73
14 Echidna                  0x576c80=e9bbe61000
15 Angelo Credo             0x576c80=e93b361300
16 Abgelo Agnus             0x576c80=e9db711400
17 Sanctus                  0x576c80=e96bb61800
18 Kyrie                    0x576c80=e97bcf1a00
19 Dante                    0x576c80=e9fb8c2400
*/

// mephisto 08t / 08c
/*
00 Default                  0x57f1e0=8b0d4c43e1
01 Scarecrow(Leg)           0x57f1e0=e92b06fcff
02 Scarecrow(Arm)           0x57f1e0=e92bf5fdff
03 Mega Scarecrow           0x57f1e0=e9fb05feff
04 Bianco Angelo            0x57f1e0=e92b28feff
05 Alto Angelo              0x57f1e0=e99b7affff
06 Faust                    0x57f1e0=e92b660100
07 Frost                    0x57f1e0=e97b4d0200
08 Assault                  0x57f1e0=e98b3f0300
09 Blitz                    0x57f1e0=e97b250500
10 Chimera Seed             0x57f1e0=e97bcf0500
11 Basilisk                 0x57f1e0=e9cbb50900
12 Berial                   0x57f1e0=e9db180b00
13 Angelo Credo             0x57f1e0=e9dbb01200
14 Abgelo Agnus             0x57f1e0=e97bec1300
15 Sanctus                  0x57f1e0=e90b311800
16 Dante                    0x57f1e0=e99b072400
*/

// faust 09t / 09c
/*
00 Default                  0x595810=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           0x595810=e9fb9ffaff
02 Scarecrow(Arm)           0x595810=e9fb8efcff
03 Mega Scarecrow           0x595810=e9cb9ffcff
04 Bianco Angelo            0x595810=e9fbc1fcff
05 Alto Angelo              0x595810=e96b14feff
06 Mephisto                 0x595810=e9cb99feff
07 Frost                    0x595810=e94be70000
08 Assault                  0x595810=e95bd90100
09 Blitz                    0x595810=e94bbf0300
10 Chimera Seed             0x595810=e94b690400
11 Basilisk                 0x595810=e99b4f0800
12 Berial                   0x595810=e9abb20900
13 Bael                     0x595810=e99b440b00, 0x64b77b=73
14 Echidna                  0x595810=e92bfb0e00
15 Angelo Credo             0x595810=e9ab4a1100
16 Abgelo Agnus             0x595810=e94b861200
17 Sanctus                  0x595810=e9dbca1600
18 Kyrie                    0x595810=e9ebe31800
19 Dante                    0x595810=e96ba12200
*/

// frost 0at / 0ac
/*
00 Default                  0x5a3f60=8b0d4c43e1
01 Scarecrow(Leg)           0x5a3f60=e9abb8f9ff
02 Scarecrow(Arm)           0x5a3f60=e9aba7fbff
03 Mega Scarecrow           0x5a3f60=e97bb8fbff
04 Bianco Angelo            0x5a3f60=e9abdafbff
05 Alto Angelo              0x5a3f60=e91b2dfdff
06 Mephisto                 0x5a3f60=e97bb2fdff
07 Faust                    0x5a3f60=e9ab18ffff
08 Assault                  0x5a3f60=e90bf20000
09 Blitz                    0x5a3f60=e9fbd70200
10 Chimera Seed             0x5a3f60=e9fb810300
11 Basilisk                 0x5a3f60=e94b680700
12 Berial                   0x5a3f60=e95bcb0800
13 Angelo Credo             0x5a3f60=e95b631000
14 Abgelo Agnus             0x5a3f60=e9fb9e1100
15 Sanctus                  0x5a3f60=e98be31500
16 Dante                    0x5a3f60=e91bba2100
*/

// assault 0bt / 0bc
/*
00 Default                  0x5b3170=8b0d4c43e1
01 Scarecrow(Leg)           0x5b3170=e99bc6f8ff
02 Scarecrow(Arm)           0x5b3170=e99bb5faff
03 Mega Scarecrow           0x5b3170=e96bc6faff
04 Bianco Angelo            0x5b3170=e99be8faff
05 Alto Angelo              0x5b3170=e90b3bfcff
06 Mephisto                 0x5b3170=e96bc0fcff
07 Faust                    0x5b3170=e99b26feff
08 Frost                    0x5b3170=e9eb0dffff
09 Blitz                    0x5b3170=e9ebe50100
10 Chimera Seed             0x5b3170=e9eb8f0200
11 Basilisk                 0x5b3170=e93b760600
12 Berial                   0x5b3170=e94bd90700
13 Angelo Credo             0x5b3170=e94b710f00
14 Abgelo Agnus             0x5b3170=e9ebac1000
15 Sanctus                  0x5b3170=e97bf11400
16 Dante                    0x5b3170=e90bc82000
*/

// blitz 0ct / 0cc 
/*
00 Default                  0x5d1760=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           0x5d1760=e9abe0f6ff
02 Scarecrow(Arm)           0x5d1760=e9abcff8ff
03 Mega Scarecrow           0x5d1760=e97be0f8ff
04 Bianco Angelo            0x5d1760=e9ab02f9ff
05 Alto Angelo              0x5d1760=e91b55faff
06 Mephisto                 0x5d1760=e97bdafaff
07 Faust                    0x5d1760=e9ab40fcff
08 Frost                    0x5d1760=e9fb27fdff
09 Assault                  0x5d1760=e90b1afeff
10 Chimera Seed             0x5d1760=e9fba90000
11 Basilisk                 0x5d1760=e94b900400
12 Berial                   0x5d1760=e95bf30500
13 Bael                     0x5d1760=e94b850700, 0x64b77b=73
14 Echidna                  0x5d1760=e9db3b0b00
15 Angelo Credo             0x5d1760=e95b8b0d00
16 Abgelo Agnus             0x5d1760=e9fbc60e00
17 Sanctus                  0x5d1760=e98b0b1300
18 Kyrie                    0x5d1760=e99b241500
19 Dante                    0x5d1760=e91be21e00
*/

// chimera seed 0dt / 0dc
/*
00 Default                  0x5dc160=8b0d4c43e1
01 Scarecrow(Leg)           0x5dc160=e9ab36f6ff
02 Scarecrow(Arm)           0x5dc160=e9ab25f8ff
03 Mega Scarecrow           0x5dc160=e97b36f8ff
04 Bianco Angelo            0x5dc160=e9ab58f8ff
05 Alto Angelo              0x5dc160=e91babf9ff
06 Mephisto                 0x5dc160=e97b30faff
07 Faust                    0x5dc160=e9ab96fbff
08 Frost                    0x5dc160=e9fb7dfcff
09 Assault                  0x5dc160=e90b70fdff
10 Blitz                    0x5dc160=e9fb55ffff
11 Basilisk                 0x5dc160=e94be60300
12 Berial                   0x5dc160=e95b490500
13 Angelo Credo             0x5dc160=e95be10c00
14 Abgelo Agnus             0x5dc160=e9fb1c0e00
15 Sanctus                  0x5dc160=e98b611200
16 Dante                    0x5dc160=e91b381e00
*/

// basilisk 11t / 11c
/*
00 Default                  0x61a7b0=8b0d4c43e1
01 Scarecrow(Leg)           0x61a7b0=e95b50f2ff
02 Scarecrow(Arm)           0x61a7b0=e95b3ff4ff
03 Mega Scarecrow           0x61a7b0=e92b50f4ff
04 Bianco Angelo            0x61a7b0=e95b72f4ff
05 Alto Angelo              0x61a7b0=e9cbc4f5ff
06 Mephisto                 0x61a7b0=e92b4af6ff
07 Faust                    0x61a7b0=e95bb0f7ff
08 Frost                    0x61a7b0=e9ab97f8ff
09 Assault                  0x61a7b0=e9bb89f9ff
10 Blitz                    0x61a7b0=e9ab6ffbff
11 Chimera Seed             0x61a7b0=e9ab19fcff
12 Berial                   0x61a7b0=e90b630100
13 Angelo Credo             0x61a7b0=e90bfb0800
14 Abgelo Agnus             0x61a7b0=e9ab360a00
15 Sanctus                  0x61a7b0=e93b7b0e00
16 Dante                    0x61a7b0=e9cb511a00
*/
 
// berial 12t / 12c
/*
00 Default                  0x630ac0=8b0d4c43e1
01 Angelo Credo             0x630ac0=e9fb970700
02 Abgelo Agnus             0x630ac0=e99bd30800
03 Sanctus                  0x630ac0=e92b180d00
04 Dante                    0x630ac0=e9bbee1800
*/

// bael 13t / 13c
/*
00 Default                  0x649cb0=8b0d4c43e1
01 Berial                   0x649cb0=e90b6efeff
02 Angelo Credo             0x649cb0=e90b060600
03 Abgelo Agnus             0x649cb0=e9ab410700
04 Sanctus                  0x649cb0=e93b860b00
05 Dante                    0x649cb0=e9cb5c1700
*/                          
                            
// echidna 15t / 15c        
/*                          
00 Default                  0x685340=8b0d4c43e1
01 Berial                   0x685340=e97bb7faff
02 Angelo Credo             0x685340=e97b4f0200
03 Abgelo Agnus             0x685340=e91b8b0300
04 Sanctus                  0x685340=e9abcf0700
05 Dante                    0x685340=e93ba61300
*/                          
                            
// credo 16t / 16c          
/*                          
00 Defalut                  0x6aa2c0=8b0d4c43e1, 0x64b77b=74
01 Berial                   0x6aa2c0=e9fb67f8ff
02 Bael                     0x6aa2c0=e9ebf9f9ff, 0x64b77b=73
03 Echidna                  0x6aa2c0=e97bb0fdff
04 Abgelo Agnus             0x6aa2c0=e99b3b0100
05 Sanctus                  0x6aa2c0=e92b800500
06 Dante                    0x6aa2c0=e9bb561100
*/                          
                            
// agnus 17t / 17c          
/*                          0x6bde60=8b0d4c43e1
00 Berial                   0x6bde60=e95b2cf7ff
01 Angelo Credo             0x6bde60=e95bc4feff
02 Sanctus                  0x6bde60=e98b440400
*/
