#if 0
#include "EnemyReplace.hpp"

bool EnemyReplace::modEnabled{ false };
static uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);
static uintptr_t replaceScarecrowLeg = modBase + 0x13F810;
static uintptr_t replaceScarecrowArm = modBase + 0x15E710;
static uintptr_t replaceMegaScarecrow = modBase + 0x15F7E0;
static uintptr_t replacementAddressTwo = modBase + 0x24B77B;
static uintptr_t replaceBiancoAngelo = modBase + 0x161A10;
static uintptr_t replaceAltoAngelo = modBase + 0x176C80;
static uintptr_t replaceMephisto = modBase + 0x17F1E0;
static uintptr_t replaceFaust = modBase + 0x195810;
static uintptr_t replaceFrost = modBase + 0x1A3F60;
static uintptr_t replaceAssault = modBase + 0x1B3170;
static uintptr_t replaceBlitz = modBase + 0x1D1760;
static uintptr_t replaceChimera = modBase + 0x1DC160;
static uintptr_t replaceBasilisk = modBase + 0x21A7B0;
static uintptr_t replaceBerial = modBase + 0x230AC0;
static uintptr_t replaceBael = modBase + 0x249CB0;
static uintptr_t replaceEchidna = modBase + 0x285340;
static uintptr_t replaceCredo = modBase + 0x2AA2C0;
static uintptr_t replaceAgnus = modBase + 0x2BDE60;

std::optional<std::string> EnemyReplace::onInitialize()
{
    return Mod::onInitialize();
}

void EnemyReplace::ReplaceEnemyDefault(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyOne(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xfb\xee\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xfb\x10\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\x00\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xfb\xdd\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\x8b\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\x06\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xfb\x9f\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\xb8\xf9\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\xc6\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\xe0\xf6\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x36\xf6\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\x50\xf2\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\xfb\x97\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\x0b\x6e\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x7b\xb7\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\xfb\x67\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\xe9\x5b\x2c\xf7\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyTwo(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xcb\xff\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xcb\x10\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\xef\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xfb\xcc\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\x7a\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\xf5\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xfb\x8e\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\xa7\xfb\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\xb5\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\xcf\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x25\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\x3f\xf4\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\x9b\xd3\x08\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\x0b\x06\x06\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x7b\x4f\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\xeb\xf9\xf9\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\xe9\x5b\xc4\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyThree(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xfb\x21\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xfb\x32\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\x22\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xcb\xdd\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x5b\x8b\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xfb\x05\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xcb\x9f\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x7b\xb8\xfb\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x6b\xc6\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x7b\xe0\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x7b\x36\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x2b\x50\xf4\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\x2b\x18\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\xab\x41\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x1b\x8b\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\x7b\xb0\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\xe9\x8b\x44\x04\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyFour(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x6b\x74\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x6b\x85\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x9b\x74\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "xe9\x6b\x52\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\xad\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\x28\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xfb\xc1\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\xda\xfb\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\xe8\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\x02\xf9\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x58\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\x72\xf4\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\xbb\xee\x18\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\x3b\x86\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\xab\xcf\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\x9b\x3b\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyFive(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xcb\xf9\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xcb\x0a\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xfb\xf9\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xcb\xd7\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x5b\x85\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x9b\x7a\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x6b\x14\fe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x1b\x2d\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x0b\x3b\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x1b\x55\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x1b\xab\xf9\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xcb\xc4\xf5\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\xcb\x5c\x17\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x3b\xa6\x13\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\x2b\x80\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemySix(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xfb\x5f\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xfb\x70\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\x60\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xfb\x3d\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\xeb\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\x66\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xcb\x99\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x7b\xb2\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x6b\xc0\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x7b\xda\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x7b\x30\xfa\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x2b\x4a\xf6\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\xbb\x56\x11\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemySeven(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\x47\x06\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\x58\x04\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\x47\x04\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\x25\x04\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\xd2\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\x4d\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\xe7\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\x18\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\x26\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\x40\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x96\xfb\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\xb0\xf7\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyEight(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x5b\x39\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x5b\x4a\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x8b\x39\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x5b\x17\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xeb\xc4\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x8b\x3f\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x5b\xd9\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x0b\xf2\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\x0d\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xfb\x27\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xfb\x7d\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x97\xf8\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyNine(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\x1f\x09\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\x30\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\x1f\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\xfd\x06\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\xaa\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\x25\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\xbf\x03\00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xfb\xd7\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\xe5\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x0b\x1a\xfe\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x0b\x70\xfd\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xbb\x89\xf9\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyTen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\xc9\x09\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\xda\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\xc9\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\xa7\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\x54\x06\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\xcf\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\x69\x04\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xfb\x81\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\x8f\x02\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xfb\xa9\x00\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xfb\x55\xff\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x6f\xfb\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyEleven(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x9b\xaf\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x9b\xc0\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xcb\xaf\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x9b\x8d\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x2b\x3b\x0a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xcb\xb5\x09\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x9b\x4f\x08\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "xe9\x4b\x68\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x3b\x76\x06\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x4b\x90\x04\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x4b\xe6\x03\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x19\xfc\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyTwelve(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xab\x12\x0f\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xab\x23\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xdb\x12\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xab\xf0\x0c\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x3b\x9e\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xdb\x18\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xab\xb2\x09\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x5b\xcb\x08\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x4b\xd9\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x5b\xf3\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x5b\x49\x05\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x0b\x63\x01\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyThirteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xab\xaa\x16\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xab\xbb\x14\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xcb\xa4\x0e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xab\x88\x14\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x2b\x30\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xdb\xb0\x12\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x9b\x44\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x5b\x63\x10\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x4b\x71\x0f\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x4b\x85\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x5b\xe1\x0c\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x0b\xfb\x08\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyFourteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\xe6\x17\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\xf7\x15\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "xe9\x5b\x5b\x12\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\xc4\x15\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xbb\xe6\x10\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\xec\x13\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x2b\xfb\x0e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xfb\x9e\x11\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\xac\x10\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xdb\x3b\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xfb\x1c\x0e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x36\x0a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyFifteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xdb\x2a\x1c\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xdb\x3b\x1a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xdb\xaa\x14\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xdb\x08\x1a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x3b\x36\x13\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x0b\x31\x18\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xab\x4a\x11\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x8b\xe3\x15\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x7b\xf1\x14\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x5b\x8b\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x8b\x61\x12\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x3b\x7b\x0e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemySixteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x6b\x01\x28\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x6b\x12\x26\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\xe6\x15\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x6b\xdf\x25\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\x71\x14\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x9b\x07\x24\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\x86\x12\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x1b\xba\x21\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x0b\xc8\x20\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xfb\xc6\x0e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x1b\x38\x1e<x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xcb\x51\x1a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemySeventeen(uintptr_t address)
{
    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x0b\x2b\x1a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x6b\xb6\x18\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xdb\xca\x16\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x8b\x0b\x13\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyEighteen(uintptr_t address)
{
    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x1b\x44\x1c\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x7b\xcf\x1a\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xeb\xe3\x18\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x9b\x24\x15\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}

void EnemyReplace::ReplaceEnemyNineteen(uintptr_t address)
{
    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x9b\x01\x26\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xfb\x8c\x24\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x6b\xa1\x22\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x1b\xe2\x1e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }
}


void EnemyReplace::onGUIframe()
{
    if (ImGui::CollapsingHeader("Enemy Replacement"))
    {
        ImGui::Spacing();

        ImGui::Text("No Cyclical Replacements!");
        ImGui::SameLine();
        HelpMarker("Example: Don't replace Mephisto with Faust and Faust with Mephisto at the same time. "
                   "The game will get stuck in a loop and cause undesired behaviour.");

        ImGui::Spacing();

        ImGui::Text("Replace Scarecrow(Leg) with: ");
        {
            const char* scarecrowLeg_items[] = {
                "Default", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo", "Alto Angelo",  "Mephisto",
                "Faust",   "Frost",          "Assault",        "Blitz",         "Chimera Seed", "Basilisk",
                "Berial",  "Credo",          "Agnus",          "Sanctus",       "Dante",
            };
            static int scarecrowLeg_current = 0;
            if (ImGui::Combo("##Replace SC(L)", &scarecrowLeg_current, scarecrowLeg_items,
                             IM_ARRAYSIZE(scarecrowLeg_items)))
            {
                switch (scarecrowLeg_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceScarecrowLeg);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceScarecrowLeg);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceScarecrowLeg);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceScarecrowLeg);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceScarecrowLeg);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceScarecrowLeg);
                    break;
                case 6:
                    ReplaceEnemySix(replaceScarecrowLeg);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceScarecrowLeg);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceScarecrowLeg);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceScarecrowLeg);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceScarecrowLeg);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceScarecrowLeg);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceScarecrowLeg);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceScarecrowLeg);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceScarecrowLeg);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceScarecrowLeg);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceScarecrowLeg);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Scarecrow(Arm) with: ");
        {
            const char* scarecrowArm_items[] = { "Default",     "Scarecrow(Leg)", "Mega Scarecrow", "Bianco Angelo",
                                                 "Alto Angelo", "Mephisto",       "Faust",          "Frost",
                                                 "Assault",     "Blitz",          "Chimera Seed",   "Basilisk",
                                                 "Berial",      "Credo",          "Agnus",          "Sanctus",
                                                 "Dante" };
            static int scarecrowArm_current = 0;
            if (ImGui::Combo("##Replace SC(A)", &scarecrowArm_current, scarecrowArm_items,
                             IM_ARRAYSIZE(scarecrowArm_items)))
            {
                switch (scarecrowArm_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceScarecrowArm);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceScarecrowArm);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceScarecrowArm);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceScarecrowArm);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceScarecrowArm);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceScarecrowArm);
                    break;
                case 6:
                    ReplaceEnemySix(replaceScarecrowArm);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceScarecrowArm);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceScarecrowArm);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceScarecrowArm);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceScarecrowArm);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceScarecrowArm);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceScarecrowArm);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceScarecrowArm);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceScarecrowArm);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceScarecrowArm);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceScarecrowArm);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Mega Scarecrow with: ");
        {
            const char* megaScarecrow_items[] = { "Default",     "Scarecrow(Leg)", "Scarecrow(Arm)", "Bianco Angelo",
                                                  "Alto Angelo", "Mephisto",       "Faust",          "Frost",
                                                  "Assault",     "Blitz",          "Chimera Seed",   "Basilisk",
                                                  "Berial",      "Bael",           "Echidna",        "Credo",
                                                  "Agnus",       "Sanctus",        "Kyrie",          "Dante" };
            static int megaScarecrow_current = 0;
            if (ImGui::Combo("##Replace MSC", &megaScarecrow_current, megaScarecrow_items,
                             IM_ARRAYSIZE(megaScarecrow_items)))
            {
                switch (megaScarecrow_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceMegaScarecrow);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceMegaScarecrow);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceMegaScarecrow);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceMegaScarecrow);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceMegaScarecrow);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceMegaScarecrow);
                    break;
                case 6:
                    ReplaceEnemySix(replaceMegaScarecrow);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceMegaScarecrow);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceMegaScarecrow);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceMegaScarecrow);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceMegaScarecrow);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceMegaScarecrow);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceMegaScarecrow);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceMegaScarecrow);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceMegaScarecrow);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceMegaScarecrow);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceMegaScarecrow);
                    break;
                case 17:
                    ReplaceEnemySeventeen(replaceMegaScarecrow);
                    break;
                case 18:
                    ReplaceEnemyEighteen(replaceMegaScarecrow);
                    break;
                case 19:
                    ReplaceEnemyNineteen(replaceMegaScarecrow);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Bianco Angelo with: ");
        {
            const char* bianco_items[] = { "Default",     "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                           "Alto Angelo", "Mephisto",       "Faust",          "Frost",
                                           "Assault",     "Blitz",          "Chimera Seed",   "Basilisk",
                                           "Berial",      "Credo",          "Agnus",          "Sanctus",
                                           "Dante" };
            static int bianco_current = 0;
            if (ImGui::Combo("##Replace Bianco", &bianco_current, bianco_items, IM_ARRAYSIZE(bianco_items)))
            {
                switch (bianco_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceBiancoAngelo);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceBiancoAngelo);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceBiancoAngelo);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceBiancoAngelo);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceBiancoAngelo);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceBiancoAngelo);
                    break;
                case 6:
                    ReplaceEnemySix(replaceBiancoAngelo);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceBiancoAngelo);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceBiancoAngelo);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceBiancoAngelo);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceBiancoAngelo);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceBiancoAngelo);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceBiancoAngelo);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceBiancoAngelo);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceBiancoAngelo);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceBiancoAngelo);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceBiancoAngelo);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Alto Angelo with: ");
        {
            const char* alto_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                         "Bianco Angelo", "Mephisto",       "Faust",          "Frost",
                                         "Assault",       "Blitz",          "Chimera Seed",   "Basilisk",
                                         "Berial",        "Bael",           "Echidna",        "Credo",
                                         "Agnus",         "Sanctus",        "Kyrie",          "Dante" };
            static int alto_current = 0;
            if (ImGui::Combo("##Replace Alto", &alto_current, alto_items, IM_ARRAYSIZE(alto_items)))
            {
                switch (alto_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceAltoAngelo);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceAltoAngelo);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceAltoAngelo);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceAltoAngelo);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceAltoAngelo);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceAltoAngelo);
                    break;
                case 6:
                    ReplaceEnemySix(replaceAltoAngelo);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceAltoAngelo);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceAltoAngelo);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceAltoAngelo);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceAltoAngelo);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceAltoAngelo);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceAltoAngelo);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceAltoAngelo);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceAltoAngelo);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceAltoAngelo);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceAltoAngelo);
                    break;
                case 17:
                    ReplaceEnemySeventeen(replaceAltoAngelo);
                    break;
                case 18:
                    ReplaceEnemyEighteen(replaceAltoAngelo);
                    break;
                case 19:
                    ReplaceEnemyNineteen(replaceAltoAngelo);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Mephisto with: ");
        {
            const char* mephisto_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                             "Bianco Angelo", "Alto Angelo",    "Faust",          "Frost",
                                             "Assault",       "Blitz",          "Chimera Seed",   "Basilisk",
                                             "Berial",        "Credo",          "Agnus",          "Sanctus",
                                             "Dante" };
            static int mephisto_current = 0;
            if (ImGui::Combo("##Replace Mephisto", &mephisto_current, mephisto_items, IM_ARRAYSIZE(mephisto_items)))
            {
                switch (mephisto_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceMephisto);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceMephisto);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceMephisto);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceMephisto);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceMephisto);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceMephisto);
                    break;
                case 6:
                    ReplaceEnemySix(replaceMephisto);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceMephisto);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceMephisto);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceMephisto);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceMephisto);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceMephisto);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceMephisto);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceMephisto);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceMephisto);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceMephisto);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceMephisto);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Faust with: ");
        {
            const char* faust_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                          "Bianco Angelo", "Alto Angelo",    "Mephisto",       "Frost",
                                          "Assault",       "Blitz",          "Chimera Seed",   "Basilisk",
                                          "Berial",        "Bael",           "Echidna",        "Credo",
                                          "Agnus",         "Sanctus",        "Kyrie",          "Dante" };
            static int faust_current = 0;
            if (ImGui::Combo("##Replace Faust", &faust_current, faust_items, IM_ARRAYSIZE(faust_items)))
            {
                switch (faust_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceFaust);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceFaust);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceFaust);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceFaust);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceFaust);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceFaust);
                    break;
                case 6:
                    ReplaceEnemySix(replaceFaust);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceFaust);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceFaust);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceFaust);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceFaust);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceFaust);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceFaust);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceFaust);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceFaust);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceFaust);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceFaust);
                    break;
                case 17:
                    ReplaceEnemySeventeen(replaceFaust);
                    break;
                case 18:
                    ReplaceEnemyEighteen(replaceFaust);
                    break;
                case 19:
                    ReplaceEnemyNineteen(replaceFaust);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Frost with: ");
        {
            const char* frost_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                          "Bianco Angelo", "Alto Angelo",    "Mephisto",       "Faust",
                                          "Assault",       "Blitz",          "Chimera Seed",   "Basilisk",
                                          "Berial",        "Credo",          "Agnus",          "Sanctus",
                                          "Dante" };
            static int frost_current = 0;
            if (ImGui::Combo("##Replace Frost", &frost_current, frost_items, IM_ARRAYSIZE(frost_items)))
            {
                switch (frost_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceFrost);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceFrost);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceFrost);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceFrost);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceFrost);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceFrost);
                    break;
                case 6:
                    ReplaceEnemySix(replaceFrost);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceFrost);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceFrost);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceFrost);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceFrost);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceFrost);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceFrost);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceFrost);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceFrost);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceFrost);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceFrost);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Assault with: ");
        {
            const char* assault_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                            "Bianco Angelo", "Alto Angelo",    "Mephisto",       "Faust",
                                            "Frost",         "Blitz",          "Chimera Seed",   "Basilisk",
                                            "Berial",        "Credo",          "Agnus",          "Sanctus",
                                            "Dante" };
            static int assault_current = 0;
            if (ImGui::Combo("##Replace Assault", &assault_current, assault_items, IM_ARRAYSIZE(assault_items)))
            {
                switch (assault_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceAssault);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceAssault);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceAssault);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceAssault);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceAssault);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceAssault);
                    break;
                case 6:
                    ReplaceEnemySix(replaceAssault);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceAssault);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceAssault);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceAssault);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceAssault);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceAssault);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceAssault);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceAssault);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceAssault);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceAssault);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceAssault);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Blitz with: ");
        {
            const char* blitz_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                          "Bianco Angelo", "Alto Angelo",    "Mephisto",       "Faust",
                                          "Frost",         "Assault",        "Chimera Seed",   "Basilisk",
                                          "Berial",        "Bael",           "Echidna",        "Credo",
                                          "Agnus",         "Sanctus",        "Kyrie",          "Dante" };
            static int blitz_current = 0;
            if (ImGui::Combo("##Replace Blitz", &blitz_current, blitz_items, IM_ARRAYSIZE(blitz_items)))
            {
                switch (blitz_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceBlitz);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceBlitz);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceBlitz);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceBlitz);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceBlitz);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceBlitz);
                    break;
                case 6:
                    ReplaceEnemySix(replaceBlitz);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceBlitz);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceBlitz);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceBlitz);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceBlitz);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceBlitz);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceBlitz);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceBlitz);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceBlitz);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceBlitz);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceBlitz);
                    break;
                case 17:
                    ReplaceEnemySeventeen(replaceBlitz);
                    break;
                case 18:
                    ReplaceEnemyEighteen(replaceBlitz);
                    break;
                case 19:
                    ReplaceEnemyNineteen(replaceBlitz);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Chimera Seed with: ");
        {
            const char* chimera_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                            "Bianco Angelo", "Alto Angelo",    "Mephisto",       "Faust",
                                            "Frost",         "Assault",        "Blitz",          "Basilisk",
                                            "Berial",        "Credo",          "Agnus",          "Sanctus",
                                            "Dante" };
            static int chimera_current = 0;
            if (ImGui::Combo("##Replace Seed", &chimera_current, chimera_items, IM_ARRAYSIZE(chimera_items)))
            {
                switch (chimera_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceChimera);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceChimera);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceChimera);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceChimera);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceChimera);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceChimera);
                    break;
                case 6:
                    ReplaceEnemySix(replaceChimera);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceChimera);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceChimera);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceChimera);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceChimera);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceChimera);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceChimera);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceChimera);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceChimera);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceChimera);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceChimera);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Basilisk with: ");
        {
            const char* basilisk_items[] = { "Default",       "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow",
                                             "Bianco Angelo", "Alto Angelo",    "Mephisto",       "Faust",
                                             "Frost",         "Assault",        "Blitz",          "Chimera Seed",
                                             "Berial",        "Credo",          "Agnus",          "Sanctus",
                                             "Dante" };
            static int basilisk_current = 0;
            if (ImGui::Combo("##Replace Basilisk", &basilisk_current, basilisk_items, IM_ARRAYSIZE(basilisk_items)))
            {
                switch (basilisk_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceBasilisk);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceBasilisk);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceBasilisk);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceBasilisk);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceBasilisk);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceBasilisk);
                    break;
                case 6:
                    ReplaceEnemySix(replaceBasilisk);
                    break;
                case 7:
                    ReplaceEnemySeven(replaceBasilisk);
                    break;
                case 8:
                    ReplaceEnemyEight(replaceBasilisk);
                    break;
                case 9:
                    ReplaceEnemyNine(replaceBasilisk);
                    break;
                case 10:
                    ReplaceEnemyTen(replaceBasilisk);
                    break;
                case 11:
                    ReplaceEnemyEleven(replaceBasilisk);
                    break;
                case 12:
                    ReplaceEnemyTwelve(replaceBasilisk);
                    break;
                case 13:
                    ReplaceEnemyThirteen(replaceBasilisk);
                    break;
                case 14:
                    ReplaceEnemyFourteen(replaceBasilisk);
                    break;
                case 15:
                    ReplaceEnemyFifteen(replaceBasilisk);
                    break;
                case 16:
                    ReplaceEnemySixteen(replaceBasilisk);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Berial with: ");
        {
            const char* berial_items[] = { "Default", "Credo", "Agnus", "Sanctus", "Dante" };
            static int berial_current = 0;
            if (ImGui::Combo("##Replace Berial", &berial_current, berial_items, IM_ARRAYSIZE(berial_items)))
            {
                switch (berial_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceBerial);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceBerial);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceBerial);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceBerial);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceBerial);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Bael with: ");
        {
            const char* bael_items[] = { "Default", "Berial", "Credo", "Agnus", "Sanctus", "Dante" };
            static int bael_current = 0;
            if (ImGui::Combo("##Replace Bael", &bael_current, bael_items, IM_ARRAYSIZE(bael_items)))
            {
                switch (bael_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceBael);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceBael);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceBael);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceBael);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceBael);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceBael);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Echidna with: ");
        {
            const char* echidna_items[] = { "Default", "Berial", "Credo", "Agnus", "Sanctus", "Dante" };
            static int echidna_current = 0;
            if (ImGui::Combo("##Replace Echidna", &echidna_current, echidna_items, IM_ARRAYSIZE(echidna_items)))
            {
                switch (echidna_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceEchidna);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceEchidna);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceEchidna);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceEchidna);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceEchidna);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceEchidna);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Credo with: ");
        {
            const char* credo_items[] = { "Default", "Berial", "Bael", "Echidna", "Agnus", "Sanctus", "Dante" };
            static int credo_current = 0;
            if (ImGui::Combo("##Replace Credo", &credo_current, credo_items, IM_ARRAYSIZE(credo_items)))
            {
                switch (credo_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceCredo);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceCredo);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceCredo);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceCredo);
                    break;
                case 4:
                    ReplaceEnemyFour(replaceCredo);
                    break;
                case 5:
                    ReplaceEnemyFive(replaceCredo);
                    break;
                case 6:
                    ReplaceEnemySix(replaceCredo);
                    break;
                }
            }
        }

        ImGui::Spacing();

        ImGui::Text("Replace Agnus with: ");
        {
            const char* agnus_items[] = { "Default", "Berial", "Credo", "Sanctus" };
            static int agnus_current = 0;
            if (ImGui::Combo("##Replace Agnus", &agnus_current, agnus_items, IM_ARRAYSIZE(agnus_items)))
            {
                switch (agnus_current)
                {
                case 0:
                    ReplaceEnemyDefault(replaceAgnus);
                    break;
                case 1:
                    ReplaceEnemyOne(replaceAgnus);
                    break;
                case 2:
                    ReplaceEnemyTwo(replaceAgnus);
                    break;
                case 3:
                    ReplaceEnemyThree(replaceAgnus);
                    break;
                }
            }
        }
    }
}

void EnemyReplace::onConfigLoad(const utility::Config& cfg){
    // modEnabled = cfg.get<bool>("enemy_replace").value_or(false);
};

void EnemyReplace::onConfigSave(utility::Config& cfg){
    // cfg.set<bool>("enemy_replace", modEnabled);
};

// I rewrote this and it didn't work so I'm leaving it here as a sad memory
// fuck your memories i rewrote it again and it worked idiot
#endif
