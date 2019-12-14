#include "addresses.h"
#include "aobs.h"
#include "hacklib/Logging.h"
#include "hacklib/CrashHandler.h"

uint32_t uninit_value = 0xCCCCCCCC;

uintptr_t readMemoryPointer(uintptr_t address)
{
    auto result = 0;
    hl::CrashHandler([address, &result] { result = IsBadReadPtr((const void*)address, sizeof(uintptr_t)); },
                     [](uint32_t code) { HL_LOG_ERR("Crash prevented: %p\n", code); });
    if (result)
    {
        HL_LOG_ERR("IsBadReadPtr: %p\n", address);
        return 0;
    }
    else
        return *((uintptr_t*)address);
}
template <typename T>
T ReadPointerPath(std::vector<uintptr_t> offsets)
{
    auto len = offsets.size();
    auto ret = readMemoryPointer(offsets[0]);
    if (!ret)
        // return (T)&uninit_value;
        return NULL;
    for (uint8_t i = 1; i < len - 1; i++)
    {
        auto offset = offsets[i];
        ret = readMemoryPointer(ret + offset);
        if (!ret)
            break;
    }
    if (!ret)
        return NULL;
    else
        return (T)(ret + offsets[len - 1]);
}

// find our aobs/ addresses
void SetAddresses()
{
    styleSwitch = hl::FindPattern(styleSwitch_aob);
    swordSwitch = hl::FindPattern(swordSwitch_aob);
    gunSwitch = hl::FindPattern(gunSwitch_aob);
    jcCooldown = hl::FindPattern(jcCooldown_aob);
    movingTargetChange = hl::FindPattern(movingTargetChange_aob);
    damagemodifier = hl::FindPattern(damagemodifier_aob);
    orbDisplay = modBase + 0xFDD35;
    heightRestrictionDante = modBase + 0x3B764E;
    heightRestrictionBuster = modBase + 0x3E614B;
    heightRestrictionSplit = modBase + 0x3E5F8D;
    heightRestrictionCalibur = modBase + 0x3E6248;
    heightRestrictionExCalibur = modBase + 0x3E62B6;
    heightRestrictionSnatch = modBase + 0x3E60E4;
    heightRestrictionRaveNero = modBase + 0x3E603F;
    heightRestrictionDoubleDown = modBase + 0x3E5FE1;
    heightRestrictionRev = modBase + 0x3E4B12;
    infiniteTime = modBase + 0x94D6E;
    infiniteAllHealth = modBase + 0x11BFD9;
    disablecameraEventsOne = hl::FindPattern(disableCameraEvents1_aob);
    disablecameraEventsTwo = hl::FindPattern(disableCameraEvents2_aob);
    hideHUDOne = modBase + 0xFEFE5;
    hideHUDTwo = modBase + 0xFF1F0;
    hideHUDThree = modBase + 0x1017F8;
    enemyNoDT = modBase + 0x3309DD;
    enemyInstantDT = modBase + 0x3309A1;
    enemyInstantDTTwo = modBase + 0x3309DF;
    bpPortalAutoOpen = modBase + 0x4E1E0;
    fastPandoraOne = modBase + 0x3BCE7E;
    fastPandoraTwo = modBase + 0x3BD34F;
    fastPandoraThree = modBase + 0x3EACCA;
    fastPandoraFour = modBase + 0x3EAD76;
    fastPandoraFive = modBase + 0x3EAD98;
    fastPandoraSix = modBase + 0x3EADCB;
    infiniteAirHikeOne = modBase + 0x3ACA3A;
    infiniteAirHikeTwo = modBase + 0x3ACA4A;
    infiniteAirHikeThree = modBase + 0x3ACA51;
    infiniteDT = modBase + 0xFF315;
    infinitePlayerHealth = modBase + 0xFF282;
    berialDazeOne = modBase + 0x23515B;
    berialDazeTwo = modBase + 0x24271A;
    berialCollapse = modBase + 0x234FDA;
    bossRushOne = modBase + 0x101AA4;
    bossRushTwo = modBase + 0x4AB8E2;
    bossRushThree = modBase + 0x4AB8FD;
    bossRushFour = modBase + 0x837BAC;
    bossRushFive = modBase + 0x837CCA;
    bossRushSix = modBase + 0x837DE8;
    bossRushSeven = modBase + 0x837F07;
    difficultySelectOne = modBase + 0x4D6C7;
    difficultySelectTwo = modBase + 0x94AE7;
    difficultySelectThree = modBase + 0x94B02;
    difficultySelectFour = modBase + 0xB5689;
    difficultySelectFive = modBase + 0x11B9B6;
    difficultySelectSix = modBase + 0x11B9BE;
    difficultySelectSeven = modBase + 0x11B9DB;
    difficultySelectEight = modBase + 0x11B9E4;
    difficultySelectNine = modBase + 0x11BAFA;
    difficultySelectTen = modBase + 0x11BB02;
    difficultySelectEleven = modBase + 0x11BB19;
    difficultySelectTwelve = modBase + 0x11BB21;
    difficultySelectThirteen = modBase + 0x234D4C;
    difficultySelectFourteen = modBase + 0x24D80D;
    difficultySelectFifteen = modBase + 0x2892AB;
    difficultySelectSixteen = modBase + 0x2ABB41;
    difficultySelectSeventeen = modBase + 0x2C0435;
    difficultySelectEighteen = modBase + 0x2DA9EB;
    difficultySelectNineteen = modBase + 0x2FAD4E;
    difficultySelectTwenty = modBase + 0x304E9E;
    difficultySelectTwentyOne = modBase + 0x3309A0;
    difficultySelectTwentyTwo = modBase + 0x343B6A;
    difficultySelectTwentyThree = modBase + 0x4AB93C;
    difficultySelectTwentyFour = modBase + 0x837A5F;
    difficultySelectTwentyFive = modBase + 0x9EC8B4;
    difficultySelectTwentySix = modBase + 0x9EC9F4;
    difficultySelectTwentySeven = modBase + 0x9ECB34;
    difficultySelectTwentyEight = modBase + 0x9ECC74;
    difficultySelectTwentyNine = modBase + 0x9ECDB4;
    difficultySelectThirty = modBase + 0x11BA85;
    difficultySelectThirtyOne = modBase + 0x11BA90;
    difficultySelectThirtyTwo = modBase + 0x1D2197;
    difficultySelectThirtyThree = modBase + 0x234D5B;
    difficultySelectThirtyFour = modBase + 0x24D6F6;
    difficultySelectThirtyFive = modBase + 0x24D81C;
    difficultySelectThirtySix = modBase + 0x254383;
    difficultySelectThirtySeven = modBase + 0x2892BA;
    difficultySelectThirtyEight = modBase + 0x2949A4;
    difficultySelectThirtyNine = modBase + 0x2ABB50;
    difficultySelectForty = modBase + 0x2C0445;
    difficultySelectFortyOne = modBase + 0x2DA9CA;
    difficultySelectFortyTwo = modBase + 0x2FAD60;
    difficultySelectFortyThree = modBase + 0x304EB0;
    difficultySelectFortyFour = modBase + 0x3BFD41;
    difficultySelectFortyFive = modBase + 0x3BFD72;
    difficultySelectFortySix = modBase + 0x3C0309;
    difficultySelectFortySeven = modBase + 0x378D60;
    difficultySelectFortyEight = modBase + 0x3A7B09;
    difficultySelectFortyNine = modBase + 0x9EC0E0;
    disableDarkslayerDown = modBase + 0x3B6CE3;
    disableDarkslayerLeft = modBase + 0x3B6D42;
    disableDarkslayerRight = modBase + 0x3B6D99;
    disableDarkslayerUp = modBase + 0x3B6C84;

    // specific pointer checks for logging purposes - logs into the logfile after every boot of the game
    HL_LOG_RAW("globalSpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x28 });\n");
    globalSpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x28 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x28);
    if (globalSpeed == NULL)
    {
        HL_LOG_ERR("globalSpeed ptr is NULL");
        globalSpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("playerSpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x2c });\n");
    playerSpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x2c }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x2c);
    if (playerSpeed == NULL)
    {
        HL_LOG_ERR("playerSpeed is NULL\n");
        playerSpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("enemySpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x30 });\n");
    enemySpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x30 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x30);
    if (enemySpeed == NULL)
    {
        HL_LOG_ERR("enemySpeed is NULL\n");
        enemySpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("roomSpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x34 });\n");
    roomSpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x34 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x34);
    if (roomSpeed == NULL)
    {
        HL_LOG_ERR("roomSpeed is NULL\n");
        roomSpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("turboValue = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x38 });\n");
    turboValue =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x38 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x38);
    if (turboValue == NULL)
    {
        HL_LOG_ERR("turboValue is NULL\n");
        turboValue = (float*)&uninit_value;
    }

    moveIDAlloc = modBase + 0x43EBD6;
    selectiveCancels = modBase + 0x40332A;
    stunAnything = hl::FindPattern(stunAnything_aob);
    removeLaunchArmour = hl::FindPattern(removeLaunchArmour_aob);
    characterChangeOne = modBase + 0x3790CF;
    characterChangeTwo = modBase + 0x87825C;
    characterchangeThree = modBase + 0x4ABC19;
    characterchangeFour = modBase + 0x4ABE59;
    characterChangeFive = modBase + 0x4ABFB9;
    characterChangeSix = modBase + 0x4AC109;
    characterChangeSeven = modBase + 0x4AC259;
    characterChangeEight = modBase + 0x4AC3A9;
    characterChangeNine = modBase + 0x4AC4F9;
    characterChangeTen = modBase + 0x4AC649;
    characterChangeEleven = modBase + 0x4AC769;
    characterChangeTwelve = modBase + 0x4AC889;
    characterChangeThirteen = modBase + 0x4AC999;
    characterChangeFourteen = modBase + 0x4ACAE9;
    characterChangeFifteen = modBase + 0x4ACC39;
    characterChangeSixteen = modBase + 0x4ACD99;
    characterChangeSeventeen = modBase + 0x4ACF19;
    characterChangeEighteen = modBase + 0x4AD069;
    characterChangeNineteen = modBase + 0x4AD1D9;
    characterChangeTwenty = modBase + 0x4AD339;
    characterChangeTwentyOne = modBase + 0x4AD449;
    lockOn = modBase + 0x3a8337;
    lockOff = modBase + 0x3a838c;
    trackingFullHouse = modBase + 0x3d3471;
    timerAlloc = modBase + 0x3AD76A;
    backForward = modBase + 0x405BC1;
    trickDown = modBase + 0x3CB119;
    floorTouch = modBase + 0x3CB33D;
    infiniteTrickRange = modBase + 0x3CB0A8;
    cameraSensitivity = modBase + 0x180A8;
    roseRemovesPins = modBase + 0x4158C3;

    // we'll set this in StageJump function
    // roomID = ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830, 0x6C });
    // //(int*)(*(uintptr_t*)(*(uintptr_t*)(modBase + 0xA552C8) + 0x3830) + 0x6C);
    bpFloorStage =
        ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830,
                                0x74 }); //(int*)(*(uintptr_t*)(*(uintptr_t*)(modBase + 0xA552C8) + 0x3830) + 0x74);
    if (bpFloorStage == NULL)
    {
        HL_LOG_ERR("bpFloorStage is NULL\n");
        bpFloorStage = (int*)&uninit_value;
    }
    initiateJump =
        ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830,
                                0x68 }); //(int*)(*(uintptr_t*)(*(uintptr_t*)(modBase + 0xA552C8) + 0x3830) + 0x68);
    if (initiateJump == NULL)
    {
        HL_LOG_ERR("initiateJump is NULL\n");
        initiateJump = (int*)&uninit_value;
    }

    sprintFasterActivate = modBase + 0x40456C;
    enemyAttackOffscreen = modBase + 0xA8CE9;
    slowWalkOne = modBase + 0x421C83;
    slowWalkTwo = modBase + 0x421D85;
    cameraHeightSetting = modBase + 0x191C5;
    cameraDistanceSetting = modBase + 0x1EF09;
    cameraDistanceLockonSetting = modBase + 0x19D16;
    cameraAngleSetting = modBase + 0x1914C;
    hideStyle = modBase + 0x1017F8;
    hideOrbs = modBase + 0xFF1F2;
    autoSkipIntro = modBase + 0x6DF1;
    autoSkipOutro = modBase + 0x7E6D;
    ldkWithDMDOne = modBase + 0x94AED;
    ldkWithDMDTwo = modBase + 0x3309A1;
    infiniteReviveOne = modBase + 0x39FA7C;
    infiniteReviveTwo = modBase + 0xD270;
    infiniteReviveThree = modBase + 0xD49B;
    infiniteReviveFour = modBase + 0xD25A;
    replaceScarecrowLeg = modBase + 0x13F810;
    replaceScarecrowArm = modBase + 0x15E710;
    replaceMegaScarecrow = modBase + 0x15F7E0;
    replacementAddressTwo = modBase + 0x24B77B;
    replaceBiancoAngelo = modBase + 0x161A10;
    replaceAltoAngelo = modBase + 0x176C80;
    hideTimer = modBase + 0xFDE27;
    replaceMephisto = modBase + 0x17F1E0;
    replaceFaust = modBase + 0x195810;
    replaceFrost = modBase + 0x1A3F60;
    replaceAssault = modBase + 0x1B3170;
    replaceBlitz = modBase + 0x1D1760;
    replaceChimera = modBase + 0x1DC160;
    replaceBasilisk = modBase + 0x21A7B0;
    replaceBerial = modBase + 0x230AC0;
    replaceBael = modBase + 0x249CB0;
    replaceEchidna = modBase + 0x285340;
    replaceCredo = modBase + 0x2AA2C0;
    replaceAgnus = modBase + 0x2BDE60;
    infiniteTableHopper = modBase + 0x3F873C;
}