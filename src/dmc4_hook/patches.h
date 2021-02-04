#include "main.h"
#include "naked.h"

//patch functions that are being applied by patching the bytes in the specific addresses according to function body

void hlMain::ReplaceEnemyDefault(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\x8b\x0d\x4c\x43\xe1\x00", 6);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x74\x12", 2);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\x8b\x0d\x4c\x43\xe1\x00", 6);
    }
}

void hlMain::ReplaceEnemyOne(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xfb\xee\x01\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xfb\x10\xfe\xff", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\x00\xfe\xff", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xfb\xdd\xfd\xff", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\x8b\xfc\xff", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\x06\xfc\xff", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xfb\x9f\xfa\xff", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\xb8\xf9\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\xc6\xf8\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\xe0\xf6\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x36\xf6\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\x50\xf2\xff", 5);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\xfb\x97\x07\x00", 5);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\x0b\x6e\xfe\xff", 5);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x7b\xb7\xfa\xff", 5);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\xfb\x67\xf8\xff", 5);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\xe9\x5b\x2c\xf7\xff", 5);
    }
}

void hlMain::ReplaceEnemyTwo(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xcb\xff\x01\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xcb\x10\x00\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\xef\xff\xff", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xfb\xcc\xff\xff", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\x7a\xfe\xff", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\xf5\xfd\xff", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xfb\x8e\xfc\xff", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\xa7\xfb\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\xb5\xfa\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\xcf\xf8\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x25\xf8\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\x3f\xf4\xff", 5);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\x9b\xd3\x08\x00", 5);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\x0b\x06\x06\x00", 5);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x7b\x4f\x02\x00", 5);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\xeb\xf9\xf9\xff", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\xe9\x5b\xc4\xfe\xff", 5);
    }
}

void hlMain::ReplaceEnemyThree(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xfb\x21\x02\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xfb\x32\x00\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\x22\x00\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xcb\xdd\xff\xff", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x5b\x8b\xfe\xff", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xfb\x05\xfe\xff", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xcb\x9f\xfc\xff", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x7b\xb8\xfb\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x6b\xc6\xfa\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x7b\xe0\xf8\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x7b\x36\xf8\ff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x2b\x50\xf4\xff", 5);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\x2b\x18\x0d\x00", 5);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\xab\x41\x07\x00", 5);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x1b\x8b\x03\x00", 5);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\x7b\xb0\xfd\xff", 5);
    }

    if (address == replaceAgnus)
    {
        replaceAgnus_patch.apply(replaceAgnus, "\xe9\x8b\x44\x04\x00", 5);
    }
}

void hlMain::ReplaceEnemyFour(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x6b\x74\x03\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x6b\x85\x01\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x9b\x74\x01\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "xe9\x6b\x52\x01\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\xad\xfe\xff", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\x28\xfe\xff", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xfb\xc1\xfc\xff", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\xda\xfb\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\xe8\xfa\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\x02\xf9\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x58\xf8\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\x72\xf4\xff", 5);
    }

    if (address == replaceBerial)
    {
        replaceBerial_patch.apply(replaceBerial, "\xe9\xbb\xee\x18\x00", 5);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\x3b\x86\x0b\x00", 5);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\xab\xcf\x07\x00", 5);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\x9b\x3b\x01\x00", 5);
    }
}

void hlMain::ReplaceEnemyFive(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xcb\xf9\x03\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xcb\x0a\x02\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xfb\xf9\x01\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xcb\xd7\x01\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x5b\x85\x00\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x9b\x7a\xff\xff", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x6b\x14\fe\xff", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x1b\x2d\xfd\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x0b\x3b\xfc\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x1b\x55\xfa\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x1b\xab\xf9\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xcb\xc4\xf5\xff", 5);
    }

    if (address == replaceBael)
    {
        replaceBael_patch.apply(replaceBael, "\xe9\xcb\x5c\x17\x00", 5);
    }

    if (address == replaceEchidna)
    {
        replaceEchidna_patch.apply(replaceEchidna, "\xe9\x3b\xa6\x13\x00", 5);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\x2b\x80\x05\x00", 5);
    }
}

void hlMain::ReplaceEnemySix(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xfb\x5f\x05\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xfb\x70\x03\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x2b\x60\x03\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xfb\x3d\x03\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x8b\xeb\x01\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x2b\x66\x01\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xcb\x99\xfe\xff", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x7b\xb2\xfd\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x6b\xc0\xfc\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x7b\xda\xfa\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x7b\x30\xfa\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x2b\x4a\xf6\xff", 5);
    }

    if (address == replaceCredo)
    {
        replaceCredo_patch.apply(replaceCredo, "\xe9\xbb\x56\x11\x00", 5);
    }
}

void hlMain::ReplaceEnemySeven(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\x47\x06\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\x58\x04\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\x47\x04\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\x25\x04\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\xd2\x02\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\x4d\x02\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\xe7\x00\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xab\x18\xff\xff", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x9b\x26\xfe\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xab\x40\xfc\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xab\x96\xfb\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x5b\xb0\xf7\xff", 5);
    }
}

void hlMain::ReplaceEnemyEight(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x5b\x39\x07\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x5b\x4a\x05\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x8b\x39\x05\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x5b\x17\x05\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xeb\xc4\x03\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x8b\x3f\x03\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x5b\xd9\x01\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x0b\xf2\x00\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\x0d\xff\xff", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xfb\x27\xfd\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xfb\x7d\xfc\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x97\xf8\xff", 5);
    }
}

void hlMain::ReplaceEnemyNine(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\x1f\x09\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\x30\x07\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\x1f\x07\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\xfd\x06\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\xaa\x05\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\x25\x05\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\xbf\x03\00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xfb\xd7\x02\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\xe5\x01\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x0b\x1a\xfe\xff", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x0b\x70\xfd\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xbb\x89\xf9\xff", 5);
    }
}

void hlMain::ReplaceEnemyTen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\xc9\x09\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\xda\x07\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\xc9\x07\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\xa7\x07\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\x54\x06\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\xcf\x05\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\x69\x04\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xfb\x81\x03\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\x8f\x02\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xfb\xa9\x00\x00", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xfb\x55\xff\xff", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x6f\xfb\xff", 5);
    }
}

void hlMain::ReplaceEnemyEleven(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x9b\xaf\x0d\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x9b\xc0\x0b\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xcb\xaf\x0b\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x9b\x8d\x0b\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x2b\x3b\x0a\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xcb\xb5\x09\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x9b\x4f\x08\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "xe9\x4b\x68\x07\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x3b\x76\x06\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x4b\x90\x04\x00", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x4b\xe6\x03\x00", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x19\xfc\xff", 5);
    }
}

void hlMain::ReplaceEnemyTwelve(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xab\x12\x0f\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xab\x23\x0d\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xdb\x12\x0d\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xab\xf0\x0c\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x3b\x9e\x0b\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xdb\x18\x0b\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xab\xb2\x09\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x5b\xcb\x08\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x4b\xd9\x07\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x5b\xf3\x05\x00", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x5b\x49\x05\x00", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x0b\x63\x01\x00", 5);
    }
}

void hlMain::ReplaceEnemyThirteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xab\xaa\x16\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xab\xbb\x14\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xcb\xa4\x0e\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xab\x88\x14\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x2b\x30\x0d\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\xdb\xb0\x12\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x9b\x44\x0b\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x5b\x63\x10\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x4b\x71\x0f\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x4b\x85\x07\x00", 5);
        replacementAddressTwo_patch.apply(replacementAddressTwo, "\x73\x12", 2);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x5b\xe1\x0c\x00", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x0b\xfb\x08\x00", 5);
    }
}

void hlMain::ReplaceEnemyFourteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x4b\xe6\x17\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x4b\xf7\x15\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "xe9\x5b\x5b\x12\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x4b\xc4\x15\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xbb\xe6\x10\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x7b\xec\x13\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x2b\xfb\x0e\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\xfb\x9e\x11\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\xeb\xac\x10\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xdb\x3b\x0b\x00", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\xfb\x1c\x0e\x00", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xab\x36\x0a\x00", 5);
    }
}

void hlMain::ReplaceEnemyFifteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\xdb\x2a\x1c\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\xdb\x3b\x1a\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\xdb\xaa\x14\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\xdb\x08\x1a\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x3b\x36\x13\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x0b\x31\x18\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xab\x4a\x11\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x8b\xe3\x15\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x7b\xf1\x14\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x5b\x8b\x0d\x00", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x8b\x61\x12\x00", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\x3b\x7b\x0e\x00", 5);
    }
}

void hlMain::ReplaceEnemySixteen(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        replaceScarecrowLeg_patch.apply(replaceScarecrowLeg, "\xe9\x6b\x01\x28\x00", 5);
    }

    if (address == replaceScarecrowArm)
    {
        replaceScarecrowArm_patch.apply(replaceScarecrowArm, "\xe9\x6b\x12\x26\x00", 5);
    }

    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x7b\xe6\x15\x00", 5);
    }

    if (address == replaceBiancoAngelo)
    {
        replaceBiancoAngelo_patch.apply(replaceBiancoAngelo, "\xe9\x6b\xdf\x25\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xdb\x71\x14\x00", 5);
    }

    if (address == replaceMephisto)
    {
        replaceMephisto_patch.apply(replaceMephisto, "\xe9\x9b\x07\x24\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x4b\x86\x12\x00", 5);
    }

    if (address == replaceFrost)
    {
        replaceFrost_patch.apply(replaceFrost, "\xe9\x1b\xba\x21\x00", 5);
    }

    if (address == replaceAssault)
    {
        replaceAssault_patch.apply(replaceAssault, "\xe9\x0b\xc8\x20\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\xfb\xc6\x0e\x00", 5);
    }

    if (address == replaceChimera)
    {
        replaceChimera_patch.apply(replaceChimera, "\xe9\x1b\x38\x1e<x00", 5);
    }

    if (address == replaceBasilisk)
    {
        replaceBasilisk_patch.apply(replaceBasilisk, "\xe9\xcb\x51\x1a\x00", 5);
    }
}

void hlMain::ReplaceEnemySeventeeen(uintptr_t address)
{
    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x0b\x2b\x1a\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x6b\xb6\x18\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xdb\xca\x16\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x8b\x0b\x13\x00", 5);
    }
}

void hlMain::ReplaceEnemyEighteen(uintptr_t address)
{
    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x1b\x44\x1c\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\x7b\xcf\x1a\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\xeb\xe3\x18\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x9b\x24\x15\x00", 5);
    }
}

void hlMain::ReplaceEnemyNineteen(uintptr_t address)
{
    if (address == replaceMegaScarecrow)
    {
        replaceMegaScarecrow_patch.apply(replaceMegaScarecrow, "\xe9\x9b\x01\x26\x00", 5);
    }

    if (address == replaceAltoAngelo)
    {
        replaceAltoAngelo_patch.apply(replaceAltoAngelo, "\xe9\xfb\x8c\x24\x00", 5);
    }

    if (address == replaceFaust)
    {
        replaceFaust_patch.apply(replaceFaust, "\xe9\x6b\xa1\x22\x00", 5);
    }

    if (address == replaceBlitz)
    {
        replaceBlitz_patch.apply(replaceBlitz, "\xe9\x1b\xe2\x1e\x00", 5);
    }
}
