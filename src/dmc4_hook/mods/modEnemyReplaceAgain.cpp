#include "modEnemyReplaceAgain.hpp"

bool EnemyReplaceAgain::modEnabled{ false };
static uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);
static uintptr_t replacementAddressTwo = modBase + 0x24B77B;

static uintptr_t ScarecrowLegAddress   = modBase + 0x13F810; // 0x53F810;
static uintptr_t ScarecrowArmAddress   = modBase + 0x15E710; // 0x55E710;
static uintptr_t ScarecrowMegaAddress  = modBase + 0x15F7E0; // 0x55F7E0;
static uintptr_t AngeloBiancoAddress   = modBase + 0x161A10; // 0x561A10;
static uintptr_t AngeloAltoAddress     = modBase + 0x176C80; // 0x576C80;
static uintptr_t MephistoAddress       = modBase + 0x17F1E0; // 0x57F1E0;
static uintptr_t FaustAddress          = modBase + 0x195810; // 0x595810;
static uintptr_t FrostAddress          = modBase + 0x1A3F60; // 0x5A3F60;
static uintptr_t AssaultAddress        = modBase + 0x1B3170; // 0x5B3170;
static uintptr_t BlitzAddress          = modBase + 0x1D1760; // 0x5D1760;
static uintptr_t ChimeraSeedAddress    = modBase + 0x1DC160; // 0x5DC160;
static uintptr_t BasiliskAddress       = modBase + 0x21A7B0; // 0x61A7B0;
static uintptr_t BerialAddress         = modBase + 0x230AC0; // 0x630AC0;
static uintptr_t BaelAddress           = modBase + 0x249CB0; // 0x649CB0;
static uintptr_t EchidnaAddress        = modBase + 0x285340; // 0x685340;
static uintptr_t CredoAddress          = modBase + 0x2AA2C0; // 0x6AA2C0;
static uintptr_t AgnusAddress          = modBase + 0x2BDE60; // 0x6BDE60;
static uintptr_t SanctusAddress        = modBase + 0x2F81E0; // 0x6F81E0; // diabolical sanctus
static uintptr_t KyrieAddress          = modBase + 0x323C00; // 0x723C00;
static uintptr_t DanteAddress          = modBase + 0x3BF980; // 0x7BF980;
// static uintptr_t Sanctus2Address       = modBase + 0x2F81E0; // 0x6F81E0
// crash:
// static uintptr_t CutlassAddress        = modBase + 0x209C20; // 0x609C20;
// static uintptr_t GladiusAddress        = modBase + 0x218460; // 0x618460;

int EnemyReplaceAgain::default_enemy[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
int EnemyReplaceAgain::desired_enemy[IM_ARRAYSIZE(default_enemy)];

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

std::optional<std::string> EnemyReplaceAgain::onInitialize() {
    return Mod::onInitialize();
}

naked void ScarecrowLegNaked(void) {
	_asm { jmp dword ptr [ScarecrowLegAddress] }
}
naked void ScarecrowArmNaked(void) {
    _asm { jmp dword ptr [ScarecrowArmAddress] }
}
naked void MegaNaked(void) {
    _asm { jmp dword ptr [ScarecrowMegaAddress] }
}
naked void BiancoNaked(void) {
    _asm { jmp dword ptr [AngeloBiancoAddress] }
}
naked void AltoNaked(void) {
    _asm { jmp dword ptr [AngeloAltoAddress] }
}
naked void MephistoNaked(void) {
    _asm { jmp dword ptr [MephistoAddress] }
}
naked void FaustNaked(void) {
    _asm { jmp dword ptr [FaustAddress] }
}
naked void FrostNaked(void) {
    _asm { jmp dword ptr [FrostAddress] }
}
naked void AssaultNaked(void) {
    _asm { jmp dword ptr [AssaultAddress] }
}
naked void BlitzNaked(void) {
    _asm { jmp dword ptr [BlitzAddress] }
}
naked void ChimeraSeedNaked(void) {
    _asm { jmp dword ptr [ChimeraSeedAddress] }
}
naked void BasiliskNaked(void) {
    _asm { jmp dword ptr [BasiliskAddress] }
}
naked void BerialNaked(void) {
    _asm { jmp dword ptr [BerialAddress] }
}
naked void BaelNaked(void) {
    _asm { jmp dword ptr [BaelAddress] }
}
naked void EchidnaNaked(void) {
    _asm { jmp dword ptr [EchidnaAddress] }
}
naked void CredoNaked(void) {
    _asm { jmp dword ptr [CredoAddress] }
}
naked void AgnusNaked(void) {
    _asm { jmp dword ptr [AgnusAddress] }
}
naked void SanctusNaked(void) {
    _asm { jmp dword ptr [SanctusAddress] }
}
naked void KyrieNaked(void) {
    _asm { jmp dword ptr [KyrieAddress] }
}
naked void DanteNaked(void) {
    _asm { jmp dword ptr [DanteAddress] }
}
/*naked void CutlassNaked(void) { // broken
    _asm { jmp dword ptr [CutlassAddress] }
}
naked void GladiusNaked(void) { // broken
    _asm { jmp dword ptr [GladiusAddress] }
}*/

const char* GetBytesToPatch(int desiredEnemyID) {
    switch (desiredEnemyID) {
        case  0: return (char*)&ScarecrowLegNaked;
        case  1: return (char*)&ScarecrowArmNaked;
        case  2: return (char*)&MegaNaked;
        case  3: return (char*)&BiancoNaked;
        case  4: return (char*)&AltoNaked;
        case  5: return (char*)&MephistoNaked;
        case  6: return (char*)&FaustNaked;
        case  7: return (char*)&FrostNaked;
        case  8: return (char*)&AssaultNaked;
        case  9: return (char*)&BlitzNaked;
        case 10: return (char*)&ChimeraSeedNaked;
        case 11: return (char*)&BasiliskNaked;
        case 12: return (char*)&BerialNaked;
        case 13: return (char*)&BaelNaked;
        case 14: return (char*)&EchidnaNaked;
        case 15: return (char*)&CredoNaked;
        case 16: return (char*)&AgnusNaked;
        case 17: return (char*)&SanctusNaked;
        case 18: return (char*)&KyrieNaked;
        case 19: return (char*)&DanteNaked;
    }
}
const char* ReplacementAddressTwo74 = "\x74"; // default, written when 73 isn't needed
const char* ReplacementAddressTwo73 = "\x73";

hl::Patch* EnemyReplaceAgain::GetPatch(int enemyID) {
    switch (enemyID) {
        case  0: return &replaceScarecrowLeg_patch; break;
        case  1: return &replaceScarecrowArm_patch; break;
        case  2: return &replaceScarecrowMega_patch; break;
        case  3: return &replaceAngeloBianco_patch; break;
        case  4: return &replaceAngeloAlto_patch; break;
        case  5: return &replaceMephisto_patch; break;
        case  6: return &replaceFaust_patch; break;
        case  7: return &replaceFrost_patch; break;
        case  8: return &replaceAssault_patch; break;
        case  9: return &replaceBlitz_patch; break;
        case 10: return &replaceChimera_patch; break;
        case 11: return &replaceBasilisk_patch; break;
        case 12: return &replaceBerial_patch; break;
        case 13: return &replaceBael_patch; break;
        case 14: return &replaceEchidna_patch; break;
        case 15: return &replaceCredo_patch; break;
        case 16: return &replaceAgnus_patch; break;
        case 17: return &replaceSanctus_patch; break;
        case 18: return &replaceKyrie_patch; break;
        case 19: return &replaceDante_patch; break;
    }
}

uintptr_t EnemyReplaceAgain::GetEnemyAddress(int enemyID) {
    switch (enemyID) {
        case  0: return ScarecrowLegAddress;
        case  1: return ScarecrowArmAddress;
        case  2: return ScarecrowMegaAddress;
        case  3: return AngeloBiancoAddress;
        case  4: return AngeloAltoAddress;
        case  5: return MephistoAddress;
        case  6: return FaustAddress;
        case  7: return FrostAddress;
        case  8: return AssaultAddress;
        case  9: return BlitzAddress;
        case 10: return ChimeraSeedAddress;
        case 11: return BasiliskAddress;
        case 12: return BerialAddress;
        case 13: return BaelAddress;
        case 14: return EchidnaAddress;
        case 15: return CredoAddress;
        case 16: return AgnusAddress;
        case 17: return SanctusAddress;
        case 18: return KyrieAddress;
        case 19: return DanteAddress;
    }
}

void EnemyReplaceAgain::ReplaceEnemyWith(int currentEnemyID, int desiredEnemyID) {
    // disable jmping to self
    if (currentEnemyID == desiredEnemyID) {
        GetPatch(currentEnemyID)->revert();
    }
    else {
        GetPatch(currentEnemyID)->apply(GetEnemyAddress(currentEnemyID), GetBytesToPatch(desiredEnemyID), 6);
    }

    if (desiredEnemyID == 13 && currentEnemyID != 13) { // bael requires an extra byte set (but don't set it if already bael)
        replacementAddressTwo_patch.apply(replacementAddressTwo, ReplacementAddressTwo73, 1);
    }
    else { // default
        replacementAddressTwo_patch.apply(replacementAddressTwo, ReplacementAddressTwo74, 1);
    }
}

const char* combo_lists[] = {
    "Replace Scarecrow (Leg) with:", // 00
    "Replace Scarecrow (Arm) with:", // 01
    "Replace Mega Scarecrow with:",  // 02
    "Replace Bianco Angelo with:",   // 03
    "Replace Alto Angelo with:",     // 04
    "Replace Mephisto with:",        // 05
    "Replace Faust with:",           // 06
    "Replace Frost with:",           // 07
    "Replace Assault with:",         // 08
    "Replace Blitz with:",           // 09
    "Replace Chimera Seed with:",    // 10
    "Replace Basilisk with:",        // 11
    "Replace Berial with:",          // 12
    "Replace Bael with:",            // 13
    "Replace Echidna with:",         // 14
    "Replace Credo with:",           // 15
    "Replace Agnus with:",           // 16
    "Replace Sanctus with:",         // 17
    "Replace Kyrie with:",           // 18
    "Replace Dante with:"            // 19
};

// combo list text doesn't seem to like format text
const char* combo_list_IDs[] = {
    "##ReplaceScarecrowLeg",         // 00
    "##ReplaceScarecrowArm",         // 01
    "##ReplaceMegaScarecrow",        // 02
    "##ReplaceBiancoAngelo",         // 03
    "##ReplaceAltoAngelo",           // 04
    "##ReplaceMephisto",             // 05
    "##ReplaceFaust",                // 06
    "##ReplaceFrost",                // 07
    "##ReplaceAssault",              // 08
    "##ReplaceBlitz",                // 09
    "##ReplaceChimeraSeed",          // 10
    "##ReplaceBasilisk",             // 11
    "##ReplaceBerial",               // 12
    "##ReplaceBael",                 // 13
    "##ReplaceEchidna",              // 14
    "##ReplaceCredo",                // 15
    "##ReplaceAgnus",                // 16
    "##ReplaceSanctus",              // 17
    "##ReplaceKyrie",                // 18
    "##ReplaceDante"                 // 19
};

// neither does save/loading
const char* save_load_IDs[] = {
    "ReplaceScarecrowLeg",           // 00
    "ReplaceScarecrowArm",           // 01
    "ReplaceMegaScarecrow",          // 02
    "ReplaceBiancoAngelo",           // 03
    "ReplaceAltoAngelo",             // 04
    "ReplaceMephisto",               // 05
    "ReplaceFaust",                  // 06
    "ReplaceFrost",                  // 07
    "ReplaceAssault",                // 08
    "ReplaceBlitz",                  // 09
    "ReplaceChimeraSeed",            // 10
    "ReplaceBasilisk",               // 11
    "ReplaceBerial",                 // 12
    "ReplaceBael",                   // 13
    "ReplaceEchidna",                // 14
    "ReplaceCredo",                  // 15
    "ReplaceAgnus",                  // 16
    "ReplaceSanctus",                // 17
    "ReplaceKyrie",                  // 18
    "ReplaceDante"                   // 19
};

const char* combo_items[] = {
    "Scarecrow (Leg)",               // 00
    "Scarecrow (Arm)",               // 01
    "Mega Scarecrow",                // 02
    "Bianco Angelo",                 // 03
    "Alto Angelo",                   // 04
    "Mephisto",                      // 05
    "Faust",                         // 06
    "Frost",                         // 07
    "Assault",                       // 08
    "Blitz",                         // 09
    "Chimera Seed",                  // 10
    "Basilisk",                      // 11
    "Berial",                        // 12
    "Bael",                          // 13
    "Echidna",                       // 14
    "Credo",                         // 15
    "Agnus",                         // 16
    "Sanctus",                       // 17
    "Kyrie",                         // 18
    "Dante"                          // 19
};

void EnemyReplaceAgain::onGUIframe() {
    //if (ImGui::CollapsingHeader("Enemy Replacement")) {
    ImGui::Text("Enemy Replacement - No Cyclical Replacements!");
    ImGui::SameLine();
    HelpMarker("Example: Don't replace Mephisto with Faust and Faust with Mephisto at the same time. The game will crash.\n"
        "Many of these are untested and may also crash; I thought I'd leave them in just in case there's something that works that we had disabled before.");
    ImGui::Spacing();
    if (ImGui::Button("Reset")) {
        for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
            desired_enemy[i] = i;
            ReplaceEnemyWith(default_enemy[i], desired_enemy[i]);
        }
    }
    ImGui::Spacing();
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        ImGui::Text(combo_lists[i]);
        if (ImGui::Combo(combo_list_IDs[i], &desired_enemy[i], combo_items, IM_ARRAYSIZE(combo_items))) {
            ReplaceEnemyWith(default_enemy[i], desired_enemy[i]);
        }
        ImGui::Spacing();
    }
}

void EnemyReplaceAgain::onConfigLoad(const utils::Config& cfg) {
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        desired_enemy[i] = cfg.get<int>(save_load_IDs[i]).value_or(i);
        ReplaceEnemyWith(default_enemy[i], desired_enemy[i]);
    }
}

void EnemyReplaceAgain::onConfigSave(utils::Config& cfg) {
    for (int i = 0; i < IM_ARRAYSIZE(default_enemy); i++) {
        cfg.set<int>(save_load_IDs[i], desired_enemy[i]);
    }
}

// scarecrow leg 00t / 00c
/*
00 Default                  ,0x53f810=8b0d4c43e1
01 Scarecrow(Arm)           ,0x53f810=e9fbee0100
02 Mega Scarecrow           ,0x53f810=e9cbff0100
03 Bianco Angelo            ,0x53f810=e9fb210200
04 Alto Angelo              ,0x53f810=e96b740300
05 Mephisto                 ,0x53f810=e9cbf90300
06 Faust                    ,0x53f810=e9fb5f0500
07 Frost                    ,0x53f810=e94b470600
08 Assault                  ,0x53f810=e95b390700
09 Blitz                    ,0x53f810=e94b1f0900
10 Chimera Seed             ,0x53f810=e94bc90900
11 Basilisk                 ,0x53f810=e99baf0d00
12 Berial                   ,0x53f810=e9ab120f00
13 Angelo Credo             ,0x53f810=e9abaa1600
14 Abgelo Agnus             ,0x53f810=e94be61700
15 Sanctus                  ,0x53f810=e9db2a1c00
16 Dante                    ,0x53f810=e96b012800
*/

// scarecrow arm 01t / 01c
/*
00 Default                  ,0x55e710=8b0d4c43e1
01 Scarecrow(Leg)           ,0x55e710=e9fb10feff
02 Mega Scarecrow           ,0x55e710=e9cb100000
03 Bianco Angelo            ,0x55e710=e9fb320000
04 Alto Angelo              ,0x55e710=e96b850100
05 Mephisto                 ,0x55e710=e9cb0a0200
06 Faust                    ,0x55e710=e9fb700300
07 Frost                    ,0x55e710=e94b580400
08 Assault                  ,0x55e710=e95b4a0500
09 Blitz                    ,0x55e710=e94b300700
10 Chimera Seed             ,0x55e710=e94bda0700
11 Basilisk                 ,0x55e710=e99bc00b00
12 Berial                   ,0x55e710=e9ab230d00
13 Angelo Credo             ,0x55e710=e9abbb1400
14 Abgelo Agnus             ,0x55e710=e94bf71500
15 Sanctus                  ,0x55e710=e9db3b1a00
16 Dante                    ,0x55e710=e96b122600
*/

// scarecrow mega 03t / 03c
/*
00 Default                  ,0x55f7e0=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           ,0x55f7e0=e92b00feff
02 Scarecrow(Arm)           ,0x55f7e0=e92befffff
03 Bianco Angelo            ,0x55f7e0=e92b220000
04 Alto Angelo              ,0x55f7e0=e99b740100
05 Mephisto                 ,0x55f7e0=e9fbf90100
06 Faust                    ,0x55f7e0=e92b600300
07 Frost                    ,0x55f7e0=e97b470400
08 Assault                  ,0x55f7e0=e98b390500
09 Blitz                    ,0x55f7e0=e97b1f0700
10 Chimera Seed             ,0x55f7e0=e97bc90700
11 Basilisk                 ,0x55f7e0=e9cbaf0b00
12 Berial                   ,0x55f7e0=e9db120d00
13 Bael                     ,0x55f7e0=e9cba40e00, 0x64b77b=73
14 Echidna                  ,0x55f7e0=e95b5b1200
15 Angelo Credo             ,0x55f7e0=e9dbaa1400
16 Abgelo Agnus             ,0x55f7e0=e97be61500
17 Sanctus                  ,0x55f7e0=e90b2b1a00
18 Kyrie                    ,0x55f7e0=e91b441c00
19 Dante                    ,0x55f7e0=e99b012600
*/

// bianco 05t / 05c
/*
00 Default                  ,0x561a10=8b0d4c43e1
01 Scarecrow(Leg)           ,0x561a10=e9fbddfdff
02 Scarecrow(Arm)           ,0x561a10=e9fbccffff
03 Mega Scarecrow           ,0x561a10=e9cbddffff
04 Alto Angelo              ,0x561a10=e96b520100
05 Mephisto                 ,0x561a10=e9cbd70100
06 Faust                    ,0x561a10=e9fb3d0300
07 Frost                    ,0x561a10=e94b250400
08 Assault                  ,0x561a10=e95b170500
09 Blitz                    ,0x561a10=e94bfd0600
10 Chimera Seed             ,0x561a10=e94ba70700
11 Basilisk                 ,0x561a10=e99b8d0b00
12 Berial                   ,0x561a10=e9abf00c00
13 Angelo Credo             ,0x561a10=e9ab881400
14 Abgelo Agnus             ,0x561a10=e94bc41500
15 Sanctus                  ,0x561a10=e9db081a00
16 Dante                    ,0x561a10=e96bdf2500
*/

// alto 06t / 06c
/*
00 Default                  ,0x576c80=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           ,0x576c80=e98b8bfcff
02 Scarecrow(Arm)           ,0x576c80=e98b7afeff
03 Mega Scarecrow           ,0x576c80=e95b8bfeff
04 Bianco Angelo            ,0x576c80=e98badfeff
05 Mephisto                 ,0x576c80=e95b850000
06 Faust                    ,0x576c80=e98beb0100
07 Frost                    ,0x576c80=e9dbd20200
08 Assault                  ,0x576c80=e9ebc40300
09 Blitz                    ,0x576c80=e9dbaa0500
10 Chimera Seed             ,0x576c80=e9db540600
11 Basilisk                 ,0x576c80=e92b3b0a00
12 Berial                   ,0x576c80=e93b9e0b00
13 Bael                     ,0x576c80=e92b300d00, 0x64b77b=73
14 Echidna                  ,0x576c80=e9bbe61000
15 Angelo Credo             ,0x576c80=e93b361300
16 Abgelo Agnus             ,0x576c80=e9db711400
17 Sanctus                  ,0x576c80=e96bb61800
18 Kyrie                    ,0x576c80=e97bcf1a00
19 Dante                    ,0x576c80=e9fb8c2400
*/

// mephisto 08t / 08c
/*
00 Default                  ,0x57f1e0=8b0d4c43e1
01 Scarecrow(Leg)           ,0x57f1e0=e92b06fcff
02 Scarecrow(Arm)           ,0x57f1e0=e92bf5fdff
03 Mega Scarecrow           ,0x57f1e0=e9fb05feff
04 Bianco Angelo            ,0x57f1e0=e92b28feff
05 Alto Angelo              ,0x57f1e0=e99b7affff
06 Faust                    ,0x57f1e0=e92b660100
07 Frost                    ,0x57f1e0=e97b4d0200
08 Assault                  ,0x57f1e0=e98b3f0300
09 Blitz                    ,0x57f1e0=e97b250500
10 Chimera Seed             ,0x57f1e0=e97bcf0500
11 Basilisk                 ,0x57f1e0=e9cbb50900
12 Berial                   ,0x57f1e0=e9db180b00
13 Angelo Credo             ,0x57f1e0=e9dbb01200
14 Abgelo Agnus             ,0x57f1e0=e97bec1300
15 Sanctus                  ,0x57f1e0=e90b311800
16 Dante                    ,0x57f1e0=e99b072400
*/

// faust 09t / 09c
/*
00 Default                  ,0x595810=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           ,0x595810=e9fb9ffaff
02 Scarecrow(Arm)           ,0x595810=e9fb8efcff
03 Mega Scarecrow           ,0x595810=e9cb9ffcff
04 Bianco Angelo            ,0x595810=e9fbc1fcff
05 Alto Angelo              ,0x595810=e96b14feff
06 Mephisto                 ,0x595810=e9cb99feff
07 Frost                    ,0x595810=e94be70000
08 Assault                  ,0x595810=e95bd90100
09 Blitz                    ,0x595810=e94bbf0300
10 Chimera Seed             ,0x595810=e94b690400
11 Basilisk                 ,0x595810=e99b4f0800
12 Berial                   ,0x595810=e9abb20900
13 Bael                     ,0x595810=e99b440b00, 0x64b77b=73
14 Echidna                  ,0x595810=e92bfb0e00
15 Angelo Credo             ,0x595810=e9ab4a1100
16 Abgelo Agnus             ,0x595810=e94b861200
17 Sanctus                  ,0x595810=e9dbca1600
18 Kyrie                    ,0x595810=e9ebe31800
19 Dante                    ,0x595810=e96ba12200
*/

// frost 0at / 0ac
/*
00 Default                  ,0x5a3f60=8b0d4c43e1
01 Scarecrow(Leg)           ,0x5a3f60=e9abb8f9ff
02 Scarecrow(Arm)           ,0x5a3f60=e9aba7fbff
03 Mega Scarecrow           ,0x5a3f60=e97bb8fbff
04 Bianco Angelo            ,0x5a3f60=e9abdafbff
05 Alto Angelo              ,0x5a3f60=e91b2dfdff
06 Mephisto                 ,0x5a3f60=e97bb2fdff
07 Faust                    ,0x5a3f60=e9ab18ffff
08 Assault                  ,0x5a3f60=e90bf20000
09 Blitz                    ,0x5a3f60=e9fbd70200
10 Chimera Seed             ,0x5a3f60=e9fb810300
11 Basilisk                 ,0x5a3f60=e94b680700
12 Berial                   ,0x5a3f60=e95bcb0800
13 Angelo Credo             ,0x5a3f60=e95b631000
14 Abgelo Agnus             ,0x5a3f60=e9fb9e1100
15 Sanctus                  ,0x5a3f60=e98be31500
16 Dante                    ,0x5a3f60=e91bba2100
*/

// assault 0bt / 0bc
/*
00 Default                  ,0x5b3170=8b0d4c43e1
01 Scarecrow(Leg)           ,0x5b3170=e99bc6f8ff
02 Scarecrow(Arm)           ,0x5b3170=e99bb5faff
03 Mega Scarecrow           ,0x5b3170=e96bc6faff
04 Bianco Angelo            ,0x5b3170=e99be8faff
05 Alto Angelo              ,0x5b3170=e90b3bfcff
06 Mephisto                 ,0x5b3170=e96bc0fcff
07 Faust                    ,0x5b3170=e99b26feff
08 Frost                    ,0x5b3170=e9eb0dffff
09 Blitz                    ,0x5b3170=e9ebe50100
10 Chimera Seed             ,0x5b3170=e9eb8f0200
11 Basilisk                 ,0x5b3170=e93b760600
12 Berial                   ,0x5b3170=e94bd90700
13 Angelo Credo             ,0x5b3170=e94b710f00
14 Abgelo Agnus             ,0x5b3170=e9ebac1000
15 Sanctus                  ,0x5b3170=e97bf11400
16 Dante                    ,0x5b3170=e90bc82000
*/

// blitz 0ct / 0cc 
/*
00 Default                  ,0x5d1760=8b0d4c43e1, 0x64b77b=74
01 Scarecrow(Leg)           ,0x5d1760=e9abe0f6ff
02 Scarecrow(Arm)           ,0x5d1760=e9abcff8ff
03 Mega Scarecrow           ,0x5d1760=e97be0f8ff
04 Bianco Angelo            ,0x5d1760=e9ab02f9ff
05 Alto Angelo              ,0x5d1760=e91b55faff
06 Mephisto                 ,0x5d1760=e97bdafaff
07 Faust                    ,0x5d1760=e9ab40fcff
08 Frost                    ,0x5d1760=e9fb27fdff
09 Assault                  ,0x5d1760=e90b1afeff
10 Chimera Seed             ,0x5d1760=e9fba90000
11 Basilisk                 ,0x5d1760=e94b900400
12 Berial                   ,0x5d1760=e95bf30500
13 Bael                     ,0x5d1760=e94b850700, 0x64b77b=73
14 Echidna                  ,0x5d1760=e9db3b0b00
15 Angelo Credo             ,0x5d1760=e95b8b0d00
16 Abgelo Agnus             ,0x5d1760=e9fbc60e00
17 Sanctus                  ,0x5d1760=e98b0b1300
18 Kyrie                    ,0x5d1760=e99b241500
19 Dante                    ,0x5d1760=e91be21e00
*/

// chimera seed 0dt / 0dc
/*
00 Default                  ,0x5dc160=8b0d4c43e1
01 Scarecrow(Leg)           ,0x5dc160=e9ab36f6ff
02 Scarecrow(Arm)           ,0x5dc160=e9ab25f8ff
03 Mega Scarecrow           ,0x5dc160=e97b36f8ff
04 Bianco Angelo            ,0x5dc160=e9ab58f8ff
05 Alto Angelo              ,0x5dc160=e91babf9ff
06 Mephisto                 ,0x5dc160=e97b30faff
07 Faust                    ,0x5dc160=e9ab96fbff
08 Frost                    ,0x5dc160=e9fb7dfcff
09 Assault                  ,0x5dc160=e90b70fdff
10 Blitz                    ,0x5dc160=e9fb55ffff
11 Basilisk                 ,0x5dc160=e94be60300
12 Berial                   ,0x5dc160=e95b490500
13 Angelo Credo             ,0x5dc160=e95be10c00
14 Abgelo Agnus             ,0x5dc160=e9fb1c0e00
15 Sanctus                  ,0x5dc160=e98b611200
16 Dante                    ,0x5dc160=e91b381e00
*/

// basilisk 11t / 11c
/*
00 Default                  ,0x61a7b0=8b0d4c43e1
01 Scarecrow(Leg)           ,0x61a7b0=e95b50f2ff
02 Scarecrow(Arm)           ,0x61a7b0=e95b3ff4ff
03 Mega Scarecrow           ,0x61a7b0=e92b50f4ff
04 Bianco Angelo            ,0x61a7b0=e95b72f4ff
05 Alto Angelo              ,0x61a7b0=e9cbc4f5ff
06 Mephisto                 ,0x61a7b0=e92b4af6ff
07 Faust                    ,0x61a7b0=e95bb0f7ff
08 Frost                    ,0x61a7b0=e9ab97f8ff
09 Assault                  ,0x61a7b0=e9bb89f9ff
10 Blitz                    ,0x61a7b0=e9ab6ffbff
11 Chimera Seed             ,0x61a7b0=e9ab19fcff
12 Berial                   ,0x61a7b0=e90b630100
13 Angelo Credo             ,0x61a7b0=e90bfb0800
14 Abgelo Agnus             ,0x61a7b0=e9ab360a00
15 Sanctus                  ,0x61a7b0=e93b7b0e00
16 Dante                    ,0x61a7b0=e9cb511a00
*/
 
// berial 12t / 12c
/*
00 Default                  ,0x630ac0=8b0d4c43e1
01 Angelo Credo             ,0x630ac0=e9fb970700
02 Abgelo Agnus             ,0x630ac0=e99bd30800
03 Sanctus                  ,0x630ac0=e92b180d00
04 Dante                    ,0x630ac0=e9bbee1800
*/

// bael 13t / 13c
/*
00 Default                  ,0x649cb0=8b0d4c43e1
01 Berial                   ,0x649cb0=e90b6efeff
02 Angelo Credo             ,0x649cb0=e90b060600
03 Abgelo Agnus             ,0x649cb0=e9ab410700
04 Sanctus                  ,0x649cb0=e93b860b00
05 Dante                    ,0x649cb0=e9cb5c1700
*/                          
                            
// echidna 15t / 15c        
/*                          
00 Default                  ,0x685340=8b0d4c43e1
01 Berial                   ,0x685340=e97bb7faff
02 Angelo Credo             ,0x685340=e97b4f0200
03 Abgelo Agnus             ,0x685340=e91b8b0300
04 Sanctus                  ,0x685340=e9abcf0700
05 Dante                    ,0x685340=e93ba61300
*/                          
                            
// credo 16t / 16c          
/*                          
00 Defalut                  ,0x6aa2c0=8b0d4c43e1, 0x64b77b=74
01 Berial                   ,0x6aa2c0=e9fb67f8ff
02 Bael                     ,0x6aa2c0=e9ebf9f9ff, 0x64b77b=73
03 Echidna                  ,0x6aa2c0=e97bb0fdff
04 Abgelo Agnus             ,0x6aa2c0=e99b3b0100
05 Sanctus                  ,0x6aa2c0=e92b800500
06 Dante                    ,0x6aa2c0=e9bb561100
*/                          
                            
// agnus 17t / 17c          
/*                          ,0x6bde60=8b0d4c43e1
00 Berial                   ,0x6bde60=e95b2cf7ff
01 Angelo Credo             ,0x6bde60=e95bc4feff
02 Sanctus                  ,0x6bde60=e98b440400
*/
