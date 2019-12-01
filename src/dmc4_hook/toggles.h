#include "main.h"
#include "naked.h"

void hlMain::ToggleStyleSwitch(bool toggle)
{
    if (styleSwitch)
    {
        if (toggle)
            styleSwitch_patch.apply(styleSwitch, "\x90\x90\x90\x90\x90\x90", 6);
        else
            styleSwitch_patch.revert();
    }
}

void hlMain::ToggleSwordSwitch(bool toggle)
{
    if (swordSwitch)
    {
        if (toggle)
            swordSwitch_patch.apply(swordSwitch, (char*)&limitadjust_patch, 6);
        else
            swordSwitch_patch.revert();
    }
}

void hlMain::ToggleGunSwitch(bool toggle)
{
    if (gunSwitch)
    {
        if (toggle)
            gunSwitch_patch.apply(gunSwitch, (char*)&limitadjust_patch, 6);
        else
            gunSwitch_patch.revert();
    }
}

void hlMain::ToggleJcCooldown(bool toggle)
{
    if (jcCooldown)
    {
        if (toggle)
            jcCooldown_patch.apply(jcCooldown, (char*)&limitadjust_patch, 6);
        else
            jcCooldown_patch.revert();
    }
}

void hlMain::ToggleMovingTargetChange(bool toggle)
{
    if (movingTargetChange)
    {
        if (toggle)
            movingTargetChange_patch.apply(movingTargetChange, "\x90\x90", 2);
        else
            movingTargetChange_patch.revert();
    }
}

void hlMain::ToggleHeightRestrictionDante(bool toggle)
{
    if (heightRestrictionDante)
    {
        if (toggle)
            heightRestrictionDante_patch.apply(heightRestrictionDante, "\x0F\x84\x7B\x03\x00\x00", 6);
        else
            heightRestrictionDante_patch.revert();
    }
}

void hlMain::ToggleHeightRestrictionNero(bool toggle)
{
    if (heightRestrictionBuster)
    {
        if (toggle)
            heightRestrictionBuster_patch.apply(heightRestrictionBuster, "\x0F\x84\xEF\x02\x00\x00", 6);
        else
            heightRestrictionBuster_patch.revert();
    }

    if (heightRestrictionSplit)
    {
        if (toggle)
            heightRestrictionSplit_patch.apply(heightRestrictionSplit, "\x0F\x84\xAD\x04\x00\x00", 6);
        else
            heightRestrictionSplit_patch.revert();
    }

    if (heightRestrictionCalibur)
    {
        if (toggle)
            heightRestrictionCalibur_patch.apply(heightRestrictionCalibur, "\x0F\x84\xF2\x01\x00\x00", 6);
        else
            heightRestrictionCalibur_patch.revert();
    }

    if (heightRestrictionExCalibur)
    {
        if (toggle)
            heightRestrictionExCalibur_patch.apply(heightRestrictionExCalibur, "\x0F\x84\x84\x01\x00\x00", 6);
        else
            heightRestrictionExCalibur_patch.revert();
    }

    if (heightRestrictionSnatch)
    {
        if (toggle)
            heightRestrictionSnatch_patch.apply(heightRestrictionSnatch, "\x0F\x84\x56\x03\x00\x00", 6);
        else
            heightRestrictionSnatch_patch.revert();
    }

    if (heightRestrictionRaveNero)
    {
        if (toggle)
            heightRestrictionRaveNero_patch.apply(heightRestrictionRaveNero, "\x0F\x84\xFB\x03\x00\x00", 6);
        else
            heightRestrictionRaveNero_patch.revert();
    }
}

void hlMain::ToggleInfiniteTime(bool toggle)
{
    if (infiniteTime)
    {
        if (toggle)
            infiniteTime_patch.apply(infiniteTime, "\x77\x54", 2);
        else
            infiniteTime_patch.revert();
    }
}

void hlMain::ToggleInfiniteAllHealth(bool toggle)
{
    if (infiniteAllHealth)
    {
        if (toggle)
            infiniteAllHealth_patch.apply(infiniteAllHealth, "\xF3\x0F\x5C\xC9", 4);
        else
            infiniteAllHealth_patch.revert();
    }
}

void hlMain::ToggleDisableCameraEvents(bool toggle)
{
    if (disablecameraEventsOne)
    {
        if (toggle)
            disablecameraEventsOne_patch.apply(disablecameraEventsOne, "\x31\xC0\xB8\x06\x00\x00\x00", 7);
        else
            disablecameraEventsOne_patch.revert();
    }

    if (disablecameraEventsTwo)
    {
        if (toggle)
            disablecameraEventsTwo_patch.apply(disablecameraEventsTwo, "\xEB\x17\xD9\x05\x7C\xA2\xB9\x00", 8);
        else
            disablecameraEventsTwo_patch.revert();
    }
}

void hlMain::ToggleHideHUD(bool toggle)
{
    if (hideHUDOne)
    {
        if (toggle)
            hideHUDOne_patch.apply(hideHUDOne, "\x73\x2E", 2);
        else
            hideHUDOne_patch.revert();
    }

    if (hideHUDTwo)
    {
        if (toggle)
            hideHUDTwo_patch.apply(hideHUDTwo, "\x73\x76", 2);
        else
            hideHUDTwo_patch.revert();
    }

    if (hideHUDThree)
    {
        if (toggle)
            hideHUDThree_patch.apply(hideHUDThree, "\x0F\x83\x5B\x03\x00\x00", 6);
        else
            hideHUDThree_patch.revert();
    }
}

void hlMain::ToggleEnemyNoDT(bool toggle)
{
    if (enemyNoDT)
    {
        if (toggle)
            enemyNoDT_patch.apply(enemyNoDT, "\x75\x0D", 2);
        else
            enemyNoDT_patch.revert();
    }
}

void hlMain::ToggleEnemyInstantDT(bool toggle)
{
    if (enemyInstantDTOne)
    {
        if (toggle)
            enemyInstantDTOne_patch.apply(enemyInstantDTOne, "\xEB\x1F", 2);
        else
            enemyInstantDTOne_patch.revert();
    }

    if (enemyInstantDTTwo)
    {
        if (toggle)
            enemyInstantDTTwo_patch.apply(enemyInstantDTTwo, "\x0F\x57\xC0\x90\x90", 5);
        else
            enemyInstantDTTwo_patch.revert();
    }
}

void hlMain::ToggleBpPortalAutoOpen(bool toggle)
{
    if (bpPortalAutoOpen)
    {
        if (toggle)
            bpPortalAutoOpen_patch.apply(bpPortalAutoOpen, "\x75\x07", 2);
        else
            bpPortalAutoOpen_patch.revert();
    }
}

void hlMain::ToggleFastPandora(bool toggle)
{
    if (fastPandoraOne)
    {
        if (toggle)
            fastPandoraOne_patch.apply(fastPandoraOne, "\x66\x83\xBF\x72\x17\x00\x00\x01", 8);
        else
            fastPandoraOne_patch.revert();
    }

    if (fastPandoraTwo)
    {
        if (toggle)
            fastPandoraTwo_patch.apply(fastPandoraTwo, "\x83\xBF\x7C\x52\x01\x00\x01", 7);
        else
            fastPandoraTwo_patch.revert();
    }

    if (fastPandoraThree)
    {
        if (toggle)
            fastPandoraThree_patch.apply(fastPandoraThree, "\x0F\x87\x80\x00\x00\x00", 6);
        else
            fastPandoraThree_patch.revert();
    }

    if (fastPandoraFour)
    {
        if (toggle)
            fastPandoraFour_patch.apply(fastPandoraFour, "\x77\x66", 2);
        else
            fastPandoraFour_patch.revert();
    }

    if (fastPandoraFive)
    {
        if (toggle)
            fastPandoraFive_patch.apply(fastPandoraFive, "\xD9\x05\xB4\xEE\xC0\x00", 6);
        else
            fastPandoraFive_patch.revert();
    }

    if (fastPandoraSix)
    {
        if (toggle)
            fastPandoraSix_patch.apply(fastPandoraSix, "\xD9\x05\x94\xEF\xC0\x00", 6);
        else
            fastPandoraSix_patch.revert();
    }
}

void hlMain::ToggleInfiniteAirHike(bool toggle)
{
    if (infiniteAirHikeOne)
    {
        if (toggle)
            infiniteAirHikeOne_patch.apply(infiniteAirHikeOne, "\x88\x91\x7E\x1E\x00\x00", 6);
        else
            infiniteAirHikeOne_patch.revert();
    }

    if (infiniteAirHikeTwo)
    {
        if (toggle)
            infiniteAirHikeTwo_patch.apply(infiniteAirHikeTwo, "\x80\x81\x7F\x1E\x00\x00\x00", 7);
        else
            infiniteAirHikeTwo_patch.revert();
    }

    if (infiniteAirHikeThree)
    {
        if (toggle)
            infiniteAirHikeThree_patch.apply(infiniteAirHikeThree, "\x80\x81\x7E\x1E\x00\x00\x00", 7);
        else
            infiniteAirHikeThree_patch.revert();
    }
}

void hlMain::ToggleBerialPractice(bool toggle)
{
    if (berialDazeOne)
    {
        if (toggle)
            berialDazeOne_patch.apply(berialDazeOne, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        else
            berialDazeOne_patch.revert();
    }

    if (berialCollapse)
    {
        if (toggle)
            berialCollapse_patch.apply(berialCollapse, "\x0F\x2F\xC0\x90\x90\x90\x90\x90", 8);
        else
            berialCollapse_patch.revert();
    }
}