#include "main.h"
#include "naked.h"

//patch functions that are being applied by patching the bytes in the specific addresses according to function body
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
            heightRestrictionBuster_patch.apply(heightRestrictionBuster, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionBuster_patch.revert();
    }

    if (heightRestrictionSplit)
    {
        if (toggle)
            heightRestrictionSplit_patch.apply(heightRestrictionSplit, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionSplit_patch.revert();
    }

    if (heightRestrictionCalibur)
    {
        if (toggle)
            heightRestrictionCalibur_patch.apply(heightRestrictionCalibur, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionCalibur_patch.revert();
    }

    if (heightRestrictionExCalibur)
    {
        if (toggle)
            heightRestrictionExCalibur_patch.apply(heightRestrictionExCalibur, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionExCalibur_patch.revert();
    }

    if (heightRestrictionSnatch)
    {
        if (toggle)
            heightRestrictionSnatch_patch.apply(heightRestrictionSnatch, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionSnatch_patch.revert();
    }

    if (heightRestrictionRaveNero)
    {
        if (toggle)
            heightRestrictionRaveNero_patch.apply(heightRestrictionRaveNero, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionRaveNero_patch.revert();
    }

	if (heightRestrictionDoubleDown)
    {
        if (toggle)
            heightRestrictionDoubleDown_patch.apply(heightRestrictionDoubleDown, "\x90\x90\x90\x90\x90\x90", 6);
        else
            heightRestrictionDoubleDown_patch.revert();
    }

	if (heightRestrictionRev)
    {
        if (toggle)
            heightRestrictionRev_patch.apply(heightRestrictionRev, "\x90\x90", 2);
        else
            heightRestrictionRev_patch.revert();
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
        if (toggle)
            hideOrbs_patch.apply(hideOrbs, "\x80\xbf\xfe\x00\x00\x00\x00", 7);
        else
            hideHUDTwo_patch.revert();
    }

    if (hideHUDThree)
    {
        if (toggle)
            hideHUDThree_patch.apply(hideHUDThree, "\x0F\x83\x5B\x03\x00\x00", 6);
        else
            hideHUDThree_patch.apply(hideHUDThree, "\x0F\x84\x5B\x03\x00\x00", 6);
    }

    if (hideTimer)
    {
        if (toggle)
            hideTimer_patch.apply(hideTimer, "\xc6\x40\x04\x00", 4);
        else
            hideTimer_patch.apply(hideTimer, "\xc6\x40\x04\x01", 4);
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
    if (enemyInstantDT)
    {
        if (toggle)
            enemyInstantDT_patch.apply(enemyInstantDT, "\xEB\x1f", 2);
        else
            enemyInstantDT_patch.revert();
    }

    if (enemyInstantDTTwo)
    {
        if (toggle)
            enemyInstantDTTwo_patch.apply(enemyInstantDTTwo, "\x0f\x57\xc0\x90\x90", 5);
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

void hlMain::ToggleBossRush(bool toggle)
{
    if (bossRushOne)
    {
        if (toggle)
            bossRushOne_patch.apply(bossRushOne, "\x72\x06", 2);
        else
            bossRushOne_patch.revert();
    }

    if (bossRushTwo)
    {
        if (toggle)
            bossRushTwo_patch.apply(bossRushTwo, "\x68\x65\xE5\x00\x8B", 5);
        else
            bossRushTwo_patch.revert();
    }

    if (bossRushThree)
    {
        if (toggle)
            bossRushThree_patch.apply(bossRushThree, "\x14\x00", 2);
        else
            bossRushThree_patch.revert();
    }

    if (bossRushFour)
    {
        if (toggle)
            bossRushFour_patch.apply(bossRushFour, "\x28\x00\xf8\x01", 5);
        else
            bossRushFour_patch.revert();
    }

    if (bossRushFive)
    {
        if (toggle)
            bossRushFive_patch.apply(bossRushFive, "\x3C\x00\xf9\x01", 5);
        else
            bossRushFive_patch.revert();
    }

    if (bossRushSix)
    {
        if (toggle)
            bossRushSix_patch.apply(bossRushSix, "\x50\x00\xfb\x01", 5);
        else
            bossRushSix_patch.revert();
    }

    if (bossRushSeven)
    {
        if (toggle)
            bossRushSeven_patch.apply(bossRushSeven, "\x64\x00\xfa\x01", 5);
        else
            bossRushSeven_patch.revert();
    }
}

void hlMain::SetDefault()
{
    if (difficultySelectOne)
    {
        difficultySelectOne_patch.apply(difficultySelectOne, "\x87\x78\xc7", 3);
    }

    if (difficultySelectTwo)
    {
        difficultySelectTwo_patch.apply(difficultySelectTwo, "\x14\x77", 2);
    }

    if (difficultySelectThree)
    {
        difficultySelectThree_patch.apply(difficultySelectThree, "\x28\xad\xcb\x00\x8b\xc7", 6);
    }

    if (difficultySelectFour)
    {
        difficultySelectFour_patch.apply(difficultySelectFour, "\x24\xf3\xc0\x00", 3);
    }

    if (difficultySelectFive)
    {
        difficultySelectFive_patch.apply(difficultySelectFive, "\x74\x0e", 2);
    }

    if (difficultySelectSix)
    {
        difficultySelectSix_patch.apply(difficultySelectSix, "\x14\x74\x05\xba\x02", 5);
    }

    if (difficultySelectSeven)
    {
        difficultySelectSeven_patch.apply(difficultySelectSeven, "\xfc", 1);
    }

    if (difficultySelectEight)
    {
        difficultySelectEight_patch.apply(difficultySelectEight, "\xe8\be", 1);
    }

    if (difficultySelectNine)
    {
        difficultySelectNine_patch.apply(difficultySelectNine, "\x74", 1);
    }

    if (difficultySelectTen)
    {
        difficultySelectTen_patch.apply(difficultySelectTen, "\x14\x74\x05\xb9\x02", 5);
    }

    if (difficultySelectEleven)
    {
        difficultySelectEleven_patch.apply(difficultySelectEleven, "\x24\xbf", 2);
    }

    if (difficultySelectTwelve)
    {
        difficultySelectTwelve_patch.apply(difficultySelectTwelve, "\x10\xbf", 2);
    }

    if (difficultySelectThirteen)
    {
        difficultySelectThirteen_patch.apply(difficultySelectThirteen, "\x03\x75", 2);
    }

    if (difficultySelectFourteen)
    {
        difficultySelectFourteen_patch.apply(difficultySelectFourteen, "\x03\x75", 2);
    }

    if (difficultySelectFifteen)
    {
        difficultySelectFifteen_patch.apply(difficultySelectFifteen, "\x03\x75", 2);
    }

    if (difficultySelectSixteen)
    {
        difficultySelectSixteen_patch.apply(difficultySelectSixteen, "\x03\x75", 2);
    }

    if (difficultySelectSeventeen)
    {
        difficultySelectSeventeen_patch.apply(difficultySelectSeventeen, "\x40\x01\x00\x00\x75", 5);
    }

    if (difficultySelectEighteen)
    {
        difficultySelectEighteen_patch.apply(difficultySelectEighteen, "\x03\x75", 2);
    }

    if (difficultySelectNineteen)
    {
        difficultySelectNineteen_patch.apply(difficultySelectNineteen, "\x40\x01\x00\x00\x75", 5);
    }

    if (difficultySelectTwenty)
    {
        difficultySelectTwenty_patch.apply(difficultySelectTwenty, "\x40\x01\x00\x00\x75", 5);
    }

    if (difficultySelectTwentyOne)
    {
        difficultySelectTwentyOne_patch.apply(difficultySelectTwentyOne, "\x03\x74", 2);
    }

    if (difficultySelectTwentyTwo)
    {
        difficultySelectTwentyTwo_patch.apply(difficultySelectTwentyTwo, "\x24\xf3\xc0", 3);
    }

    if (difficultySelectTwentyThree)
    {
        difficultySelectTwentyThree_patch.apply(difficultySelectTwentyThree, "\x24\xf3\xc0", 3);
    }

    if (difficultySelectTwentyFour)
    {
        difficultySelectTwentyFour_patch.apply(difficultySelectTwentyFour, "\x20\x1c", 2);
    }

    if (difficultySelectTwentyFive)
    {
        difficultySelectTwentyFive_patch.apply(difficultySelectTwentyFive, "\x00\x00\xe1\x45", 5);
    }

    if (difficultySelectTwentySix)
    {
        difficultySelectTwentySix_patch.apply(difficultySelectTwentySix, "\x00\x00\xe1\x45", 5);
    }

    if (difficultySelectTwentySeven)
    {
        difficultySelectTwentySeven_patch.apply(difficultySelectTwentySeven, "\x00\x00\xe1\x45", 5);
    }

    if (difficultySelectTwentyEight)
    {
        difficultySelectTwentyEight_patch.apply(difficultySelectTwentyEight, "\x00\x00\xe1\x45", 5);
    }

    if (difficultySelectTwentyNine)
    {
        difficultySelectTwentyNine_patch.apply(difficultySelectTwentyNine, "\x00\x00\xe1\x45", 5);
    }

    if (difficultySelectThirty)
    {
        difficultySelectThirty_patch.apply(difficultySelectThirty, "\xc4\xbf", 2);
    }

    if (difficultySelectThirtyOne)
    {
        difficultySelectThirtyOne_patch.apply(difficultySelectThirtyOne, "\xb0\xbf", 2);
    }

    if (difficultySelectThirtyTwo)
    {
        difficultySelectThirtyTwo_patch.apply(difficultySelectThirtyTwo, "\xd0\x28", 2);
    }

    if (difficultySelectThirtyThree)
    {
        difficultySelectThirtyThree_patch.apply(difficultySelectThirtyThree, "\x44", 1);
    }

    if (difficultySelectThirtyFour)
    {
        difficultySelectThirtyFour_patch.apply(difficultySelectThirtyFour, "\x24\x5b", 2);
    }

    if (difficultySelectThirtyFive)
    {
        difficultySelectThirtyFive_patch.apply(difficultySelectThirtyFive, "\x44", 1);
    }

    if (difficultySelectThirtySix)
    {
        difficultySelectThirtySix_patch.apply(difficultySelectThirtySix, "\x24\x5b", 2);
    }

    if (difficultySelectThirtySeven)
    {
        difficultySelectThirtySeven_patch.apply(difficultySelectThirtySeven, "\x44", 1);
    }

    if (difficultySelectThirtyEight)
    {
        difficultySelectThirtyEight_patch.apply(difficultySelectThirtyEight, "\x02\x00", 2);
    }

    if (difficultySelectThirtyNine)
    {
        difficultySelectThirtyNine_patch.apply(difficultySelectThirtyNine, "\x44", 1);
    }

    if (difficultySelectForty)
    {
        difficultySelectForty_patch.apply(difficultySelectForty, "\x14", 1);
    }

    if (difficultySelectFortyOne)
    {
        difficultySelectFortyOne_patch.apply(difficultySelectFortyOne, "\x5c\a0", 1);
    }

    if (difficultySelectFortyTwo)
    {
        difficultySelectFortyTwo_patch.apply(difficultySelectFortyTwo, "\x44", 1);
    }

    if (difficultySelectFortyThree)
    {
        difficultySelectFortyThree_patch.apply(difficultySelectFortyThree, "\x44", 1);
    }

    if (difficultySelectFortyFour)
    {
        difficultySelectFortyFour_patch.apply(difficultySelectFortyFour, "\xa8\x27\xd6", 3);
    }

    if (difficultySelectFortyFive)
    {
        difficultySelectFortyFive_patch.apply(difficultySelectFortyFive, "\xf0\xf2", 2);
    }

    if (difficultySelectFortySix)
    {
        difficultySelectFortySix_patch.apply(difficultySelectFortySix, "\x88\x9f", 2);
    }

    if (difficultySelectFortySeven)
    {
        difficultySelectFortySeven_patch.apply(difficultySelectFortySeven, "\x01", 1);
    }

    if (difficultySelectFortyEight)
    {
        difficultySelectFortyEight_patch.apply(difficultySelectFortyEight, "\x40", 1);
    }

    if (difficultySelectFortyNine)
    {
        difficultySelectFortyNine_patch.apply( difficultySelectFortyNine,
            "\x00\xc0\x28\x45\x00\xc0\x28\x45\x00\x00\x80\xbf\x00\xc0\xa8\x45\x00\x00\xc0\xa8\x00\x00\x61\x45\x00\x00"
            "\xe1\x45\x78\x30\x30\x00\x00\xc0\x28\x45\x00\xa0\x0c\x46\x00\xa0\x8c\x45\x00\x00\x61\x45\x00\xa0\x0c\x46"
            "\x00\x00\xe1\x44\x00\x00\x78\x30\x00\x00\x61\x45\x00\xc0\x28\x45\x00\xc0\x28\x45", 72);
    }
}

void hlMain::SetDMD()
{
    if (difficultySelectOne)
    {
        difficultySelectOne_patch.apply(difficultySelectOne, "\x05\xa8\xcd\xde\x00", 3);
    }

    if (difficultySelectTwo)
    {
        difficultySelectTwo_patch.apply(difficultySelectTwo, "\x00\x77\x14", 2);
    }

    if (difficultySelectThree)
    {
        difficultySelectThree_patch.apply(difficultySelectThree, "\x50\xb9\xcb\x00\x8b\xc7", 6);
    }

    if (difficultySelectFour)
    {
        difficultySelectFour_patch.apply(difficultySelectFour, "\x70\x01\xcc\x00", 3);
    }

    if (difficultySelectFive)
    {
        difficultySelectFive_patch.apply(difficultySelectFive, "\x72\x0e", 2);
    }

    if (difficultySelectSix)
    {
        difficultySelectSix_patch.apply(difficultySelectSix, "\x00\x74\x05\xba\x00", 5);
    }

    if (difficultySelectSeven)
    {
        difficultySelectSeven_patch.apply(difficultySelectSeven, "\xf4", 1);
    }

    if (difficultySelectEight)
    {
        difficultySelectEight_patch.apply(difficultySelectEight, "\xf4\be", 1);
    }

    if (difficultySelectNine)
    {
        difficultySelectNine_patch.apply(difficultySelectNine, "\x72", 1);
    }

    if (difficultySelectTen)
    {
        difficultySelectTen_patch.apply(difficultySelectTen, "\x00\x74\x05\xb9\x00", 5);
    }

    if (difficultySelectEleven)
    {
        difficultySelectEleven_patch.apply(difficultySelectEleven, "\x40\xbd", 2);
    }

    if (difficultySelectTwelve)
    {
        difficultySelectTwelve_patch.apply(difficultySelectTwelve, "\x40\xbd", 2);
    }

    if (difficultySelectThirteen)
    {
        difficultySelectThirteen_patch.apply(difficultySelectThirteen, "\x05\x74", 2);
    }

    if (difficultySelectFourteen)
    {
        difficultySelectFourteen_patch.apply(difficultySelectFourteen, "\x05\x74", 2);
    }

    if (difficultySelectFifteen)
    {
        difficultySelectFifteen_patch.apply(difficultySelectFifteen, "\x05\x74", 2);
    }

    if (difficultySelectSixteen)
    {
        difficultySelectSixteen_patch.apply(difficultySelectSixteen, "\x05\x74", 2);
    }

    if (difficultySelectSeventeen)
    {
        difficultySelectSeventeen_patch.apply(difficultySelectSeventeen, "\x44\x01\x00\x00\x77", 5);
    }

    if (difficultySelectEighteen)
    {
        difficultySelectEighteen_patch.apply(difficultySelectEighteen, "\x05\x74", 2);
    }

    if (difficultySelectNineteen)
    {
        difficultySelectNineteen_patch.apply(difficultySelectNineteen, "\x44\x01\x00\x00\x77", 5);
    }

    if (difficultySelectTwenty)
    {
        difficultySelectTwenty_patch.apply(difficultySelectTwenty, "\x44\x01\x00\x00\x77", 5);
    }

    if (difficultySelectTwentyOne)
    {
        difficultySelectTwentyOne_patch.apply(difficultySelectTwentyOne, "\x05\x75", 2);
    }

    if (difficultySelectTwentyTwo)
    {
        difficultySelectTwentyTwo_patch.apply(difficultySelectTwentyTwo, "\x70\x01\xcc", 3);
    }

    if (difficultySelectTwentyThree)
    {
        difficultySelectTwentyThree_patch.apply(difficultySelectTwentyThree, "\x70\x01\xcc", 3);
    }

    if (difficultySelectTwentyFour)
    {
        difficultySelectTwentyFour_patch.apply(difficultySelectTwentyFour, "\x30\x2a", 2);
    }

    if (difficultySelectTwentyFive)
    {
        difficultySelectTwentyFive_patch.apply(difficultySelectTwentyFive, "\x00\xc0\x28\x46", 5);
    }

    if (difficultySelectTwentySix)
    {
        difficultySelectTwentySix_patch.apply(difficultySelectTwentySix, "\x00\xc0\x28\x46", 5);
    }

    if (difficultySelectTwentySeven)
    {
        difficultySelectTwentySeven_patch.apply(difficultySelectTwentySeven, "\x00\xc0\x28\x46", 5);
    }

    if (difficultySelectTwentyEight)
    {
        difficultySelectTwentyEight_patch.apply(difficultySelectTwentyEight, "\x00\xc0\x28\x46", 5);
    }

    if (difficultySelectTwentyNine)
    {
        difficultySelectTwentyNine_patch.apply(difficultySelectTwentyNine, "\x00\xc0\x28\x46", 5);
    }

    if (difficultySelectThirty)
    {
        difficultySelectThirty_patch.apply(difficultySelectThirty, "\xc4\xbf", 2);
    }

    if (difficultySelectThirtyOne)
    {
        difficultySelectThirtyOne_patch.apply(difficultySelectThirtyOne, "\xb0\xbf", 2);
    }

    if (difficultySelectThirtyTwo)
    {
        difficultySelectThirtyTwo_patch.apply(difficultySelectThirtyTwo, "\xd0\x28", 2);
    }

    if (difficultySelectThirtyThree)
    {
        difficultySelectThirtyThree_patch.apply(difficultySelectThirtyThree, "\x44", 1);
    }

    if (difficultySelectThirtyFour)
    {
        difficultySelectThirtyFour_patch.apply(difficultySelectThirtyFour, "\x24\x5b", 2);
    }

    if (difficultySelectThirtyFive)
    {
        difficultySelectThirtyFive_patch.apply(difficultySelectThirtyFive, "\x44", 1);
    }

    if (difficultySelectThirtySix)
    {
        difficultySelectThirtySix_patch.apply(difficultySelectThirtySix, "\x24\x5b", 2);
    }

    if (difficultySelectThirtySeven)
    {
        difficultySelectThirtySeven_patch.apply(difficultySelectThirtySeven, "\x44", 1);
    }

    if (difficultySelectThirtyEight)
    {
        difficultySelectThirtyEight_patch.apply(difficultySelectThirtyEight, "\x02\x00", 2);
    }

    if (difficultySelectThirtyNine)
    {
        difficultySelectThirtyNine_patch.apply(difficultySelectThirtyNine, "\x44", 1);
    }

    if (difficultySelectForty)
    {
        difficultySelectForty_patch.apply(difficultySelectForty, "\x14", 1);
    }

    if (difficultySelectFortyOne)
    {
        difficultySelectFortyOne_patch.apply(difficultySelectFortyOne, "\x5c\a0", 1);
    }

    if (difficultySelectFortyTwo)
    {
        difficultySelectFortyTwo_patch.apply(difficultySelectFortyTwo, "\x44", 1);
    }

    if (difficultySelectFortyThree)
    {
        difficultySelectFortyThree_patch.apply(difficultySelectFortyThree, "\x44", 1);
    }

    if (difficultySelectFortyFour)
    {
        difficultySelectFortyFour_patch.apply(difficultySelectFortyFour, "\xa8\x27\xd6", 3);
    }

    if (difficultySelectFortyFive)
    {
        difficultySelectFortyFive_patch.apply(difficultySelectFortyFive, "\xf0\xf2", 2);
    }

    if (difficultySelectFortySix)
    {
        difficultySelectFortySix_patch.apply(difficultySelectFortySix, "\x88\x9f", 2);
    }

    if (difficultySelectFortySeven)
    {
        difficultySelectFortySeven_patch.apply(difficultySelectFortySeven, "\x01", 1);
    }

    if (difficultySelectFortyEight)
    {
        difficultySelectFortyEight_patch.apply(difficultySelectFortyEight, "\x40", 1);
    }

    if (difficultySelectFortyNine)
    {
        difficultySelectFortyNine_patch.apply(difficultySelectFortyNine,
            "\x00\xc0\x28\x45\x00\xc0\x28\x45\x00\x00\x80\xbf\x00\xc0\xa8\x45\x00\x00\xc0\xa8\x00\x00\x61\x45\x00\x00"
            "\xe1\x45\x78\x30\x30\x00\x00\xc0\x28\x45\x00\xa0\x0c\x46\x00\xa0\x8c\x45\x00\x00\x61\x45\x00\xa0\x0c\x46"
            "\x00\x00\xe1\x44\x00\x00\x78\x30\x00\x00\x61\x45\x00\xc0\x28\x45\x00\xc0\x28\x45", 72);
    }
}

void hlMain::SetGMD()
{
    if (difficultySelectOne)
    {
        difficultySelectOne_patch.apply(difficultySelectOne, "\x05\xa8\xcd\xde\x00", 3);
    }

    if (difficultySelectTwo)
    {
        difficultySelectTwo_patch.apply(difficultySelectTwo, "\x00\x77\x14", 2);
    }

    if (difficultySelectThree)
    {
        difficultySelectThree_patch.apply(difficultySelectThree, "\x50\xb9\xcb\x00\x8b\xc7", 6);
    }

    if (difficultySelectFour)
    {
        difficultySelectFour_patch.apply(difficultySelectFour, "\xc4\xcd\xde\x00", 3);
    }

    if (difficultySelectFive)
    {
        difficultySelectFive_patch.apply(difficultySelectFive, "\x72\x0e", 2);
    }

    if (difficultySelectSix)
    {
        difficultySelectSix_patch.apply(difficultySelectSix, "\x00\x74\x05\xba\x00", 5);
    }

    if (difficultySelectSeven)
    {
        difficultySelectSeven_patch.apply(difficultySelectSeven, "\x10\xbf", 2);
    }

    if (difficultySelectEight)
    {
        difficultySelectEight_patch.apply(difficultySelectEight, "\x10\xbf", 2);
    }

    if (difficultySelectNine)
    {
        difficultySelectNine_patch.apply(difficultySelectNine, "\x72", 1);
    }

    if (difficultySelectTen)
    {
        difficultySelectTen_patch.apply(difficultySelectTen, "\x00\x74\x05\xb9\x00", 5);
    }

    if (difficultySelectEleven)
    {
        difficultySelectEleven_patch.apply(difficultySelectEleven, "\x48\xbd", 2);
    }

    if (difficultySelectTwelve)
    {
        difficultySelectTwelve_patch.apply(difficultySelectTwelve, "\x48\xbd", 2);
    }

    if (difficultySelectThirteen)
    {
        difficultySelectThirteen_patch.apply(difficultySelectThirteen, "\x05\x74", 2);
    }

    if (difficultySelectFourteen)
    {
        difficultySelectFourteen_patch.apply(difficultySelectFourteen, "\x05\x74", 2);
    }

    if (difficultySelectFifteen)
    {
        difficultySelectFifteen_patch.apply(difficultySelectFifteen, "\x05\x74", 2);
    }

    if (difficultySelectSixteen)
    {
        difficultySelectSixteen_patch.apply(difficultySelectSixteen, "\x05\x74", 2);
    }

    if (difficultySelectSeventeen)
    {
        difficultySelectSeventeen_patch.apply(difficultySelectSeventeen, "\x44\x01\x00\x00\x77", 5);
    }

    if (difficultySelectEighteen)
    {
        difficultySelectEighteen_patch.apply(difficultySelectEighteen, "\x05\x74", 2);
    }

    if (difficultySelectNineteen)
    {
        difficultySelectNineteen_patch.apply(difficultySelectNineteen, "\x44\x01\x00\x00\x77", 5);
    }

    if (difficultySelectTwenty)
    {
        difficultySelectTwenty_patch.apply(difficultySelectTwenty, "\x44\x01\x00\x00\x77", 5);
    }

    if (difficultySelectTwentyOne)
    {
        difficultySelectTwentyOne_patch.apply(difficultySelectTwentyOne, "\x05\x75", 2);
    }

    if (difficultySelectTwentyTwo)
    {
        difficultySelectTwentyTwo_patch.apply(difficultySelectTwentyTwo, "\xc4\xcd\xde", 3);
    }

    if (difficultySelectTwentyThree)
    {
        difficultySelectTwentyThree_patch.apply(difficultySelectTwentyThree, "\xc4\xcd\xde", 3);
    }

    if (difficultySelectTwentyFour)
    {
        difficultySelectTwentyFour_patch.apply(difficultySelectTwentyFour, "\x50\x46", 2);
    }

    if (difficultySelectTwentyFive)
    {
        difficultySelectTwentyFive_patch.apply(difficultySelectTwentyFive, "\x00\xa0\x8c\x46", 5);
    }

    if (difficultySelectTwentySix)
    {
        difficultySelectTwentySix_patch.apply(difficultySelectTwentySix, "\x00\xa0\x8c\x46", 5);
    }

    if (difficultySelectTwentySeven)
    {
        difficultySelectTwentySeven_patch.apply(difficultySelectTwentySeven, "\x00\xa0\x8c\x46", 5);
    }

    if (difficultySelectTwentyEight)
    {
        difficultySelectTwentyEight_patch.apply(difficultySelectTwentyEight, "\x00\xa0\x8c\x46", 5);
    }

    if (difficultySelectTwentyNine)
    {
        difficultySelectTwentyNine_patch.apply(difficultySelectTwentyNine, "\x00\xa0\x8c\x46", 5);
    }

    if (difficultySelectThirty)
    {
        difficultySelectThirty_patch.apply(difficultySelectThirty, "\x04\xc0", 2);
    }

    if (difficultySelectThirtyOne)
    {
        difficultySelectThirtyOne_patch.apply(difficultySelectThirtyOne, "\x04\xc0", 2);
    }

    if (difficultySelectThirtyTwo)
    {
        difficultySelectThirtyTwo_patch.apply(difficultySelectThirtyTwo, "\x14\x2e", 2);
    }

    if (difficultySelectThirtyThree)
    {
        difficultySelectThirtyThree_patch.apply(difficultySelectThirtyThree, "\x48", 1);
    }

    if (difficultySelectThirtyFour)
    {
        difficultySelectThirtyFour_patch.apply(difficultySelectThirtyFour, "\xa4\x3e", 2);
    }

    if (difficultySelectThirtyFive)
    {
        difficultySelectThirtyFive_patch.apply(difficultySelectThirtyFive, "\x48", 1);
    }

    if (difficultySelectThirtySix)
    {
        difficultySelectThirtySix_patch.apply(difficultySelectThirtySix, "\xa4\x3e", 2);
    }

    if (difficultySelectThirtySeven)
    {
        difficultySelectThirtySeven_patch.apply(difficultySelectThirtySeven, "\x48", 1);
    }

    if (difficultySelectThirtyEight)
    {
        difficultySelectThirtyEight_patch.apply(difficultySelectThirtyEight, "\x01\x00", 2);
    }

    if (difficultySelectThirtyNine)
    {
        difficultySelectThirtyNine_patch.apply(difficultySelectThirtyNine, "\x48", 1);
    }
    if (difficultySelectForty)
    {
        difficultySelectForty_patch.apply(difficultySelectForty, "\x5a", 1);
    }

    if (difficultySelectFortyOne)
    {
        difficultySelectFortyOne_patch.apply(difficultySelectFortyOne, "\xa8\x9e", 2);
    }

    if (difficultySelectFortyTwo)
    {
        difficultySelectFortyTwo_patch.apply(difficultySelectFortyTwo, "\x48", 1);
    }

    if (difficultySelectFortyThree)
    {
        difficultySelectFortyThree_patch.apply(difficultySelectFortyThree, "\x48", 1);
    }

    if (difficultySelectFortyFour)
    {
        difficultySelectFortyFour_patch.apply(difficultySelectFortyFour, "\xf0\xf2\xc0", 3);
    }

    if (difficultySelectFortyFive)
    {
        difficultySelectFortyFive_patch.apply(difficultySelectFortyFive, "\xfc\xee", 2);
    }

    if (difficultySelectFortySix)
    {
        difficultySelectFortySix_patch.apply(difficultySelectFortySix, "\xff\x87", 2);
    }

    if (difficultySelectFortySeven)
    {
        difficultySelectFortySeven_patch.apply(difficultySelectFortySeven, "\x00", 1);
    }

    if (difficultySelectFortyEight)
    {
        difficultySelectFortyEight_patch.apply(difficultySelectFortyEight, "\x3c", 1);
    }

    if (difficultySelectFortyNine)
    {
        difficultySelectFortyNine_patch.apply(difficultySelectFortyNine,
            "\x00\xc0\xa8\x44\x00\xc0\xa8\x44\x00\x00\x80\xbf\x00\xc0\x28\x45\x00\x00\xc0\xa8\x00\x00\xe1\x44\x00\x00"
            "\x61\x45\x78\x30\x30\x00\x00\xc0\xa8\x44\x00\xa0\x8c\x45\x00\xa0\x0c\x45\x00\x00\xe1\x44\x00\xa0\x8c\x45"
            "\x00\x00\x61\x44\x00\x00\x78\x30\x00\x00\xe1\x44\x00\xc0\xa8\x44\x00\xc0\xa8\x44", 72);
    }
}

void hlMain::ToggleDisableDarkslayerDown(bool toggle)
{
    if (disableDarkslayerDown)
    {
        if (toggle)
            disableDarkslayerDown_patch.apply(disableDarkslayerDown, "\x90\x90\x90", 3);
        else
            disableDarkslayerDown_patch.revert();
    }
}

void hlMain::ToggleDisableDarkslayerLeft(bool toggle)
{
    if (disableDarkslayerLeft)
    {
        if (toggle)
            disableDarkslayerLeft_patch.apply(disableDarkslayerLeft, "\x90\x90\x90", 3);
        else
            disableDarkslayerLeft_patch.revert();
    }
}

void hlMain::ToggleDisableDarkslayerRight(bool toggle)
{
    if (disableDarkslayerRight)
    {
        if (toggle)
            disableDarkslayerRight_patch.apply(disableDarkslayerRight, "\x90\x90\x90", 3);
        else
            disableDarkslayerRight_patch.revert();
    }
}

void hlMain::ToggleDisableDarkslayerUp(bool toggle)
{
    if (disableDarkslayerUp)
    {
        if (toggle)
            disableDarkslayerUp_patch.apply(disableDarkslayerUp, "\x90\x90\x90", 3);
        else
            disableDarkslayerUp_patch.revert();
    }
}

void hlMain::ToggleStunAnything(bool toggle)
{
    if (stunAnything)
    {
        if (toggle)
            stunAnything_patch.apply(stunAnything, "\x83\xf9\x01\x90\x90", 5);
        else
            stunAnything_patch.revert();
    }
}

void hlMain::ToggleRemoveLaunchArmour(bool toggle)
{
    if (removeLaunchArmour)
    {
        if (toggle)
            removeLaunchArmour_patch.apply(removeLaunchArmour, "\x90\x90\x90\x7c\x05", 5);
        else
            removeLaunchArmour_patch.revert();
    }
}

void hlMain::ToggleCharacterChange(bool toggle)
{
    if (characterChangeOne)
    {
        if (toggle)
            characterChangeOne_patch.apply(characterChangeOne, "\x74\x46", 2);
        else
            characterChangeOne_patch.revert();
    }

    if (characterChangeTwo)
    {
        if (toggle)
            characterChangeTwo_patch.apply(characterChangeTwo, "\x01\x00", 2);
        else
            characterChangeTwo_patch.revert();
    }

    if (characterchangeThree)
    {
        if (toggle)
            characterchangeThree_patch.apply(characterchangeThree, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterchangeThree_patch.revert();
    }

    if (characterchangeFour)
    {
        if (toggle)
            characterchangeFour_patch.apply(characterchangeFour, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterchangeFour_patch.revert();
    }

    if (characterChangeFive)
    {
        if (toggle)
            characterChangeFive_patch.apply(characterChangeFive, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeFive_patch.revert();
    }

    if (characterChangeSix)
    {
        if (toggle)
            characterChangeSix_patch.apply(characterChangeSix, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeSix_patch.revert();
    }

    if (characterChangeSeven)
    {
        if (toggle)
            characterChangeSeven_patch.apply(characterChangeSeven, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeSeven_patch.revert();
    }

    if (characterChangeEight)
    {
        if (toggle)
            characterChangeEight_patch.apply(characterChangeEight, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeEight_patch.revert();
    }

    if (characterChangeNine)
    {
        if (toggle)
            characterChangeNine_patch.apply(characterChangeNine, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeNine_patch.revert();
    }

    if (characterChangeTen)
    {
        if (toggle)
            characterChangeTen_patch.apply(characterChangeTen, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeTen_patch.revert();
    }

    if (characterChangeEleven)
    {
        if (toggle)
            characterChangeEleven_patch.apply(characterChangeEleven, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeEleven_patch.revert();
    }

    if (characterChangeTwelve)
    {
        if (toggle)
            characterChangeTwelve_patch.apply(characterChangeTwelve, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeTwelve_patch.revert();
    }

    if (characterChangeThirteen)
    {
        if (toggle)
            characterChangeThirteen_patch.apply(characterChangeThirteen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeThirteen_patch.revert();
    }

    if (characterChangeFourteen)
    {
        if (toggle)
            characterChangeFourteen_patch.apply(characterChangeFourteen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeFourteen_patch.revert();
    }

    if (characterChangeFifteen)
    {
        if (toggle)
            characterChangeFifteen_patch.apply(characterChangeFifteen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeFifteen_patch.revert();
    }

    if (characterChangeSixteen)
    {
        if (toggle)
            characterChangeSixteen_patch.apply(characterChangeSixteen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeSixteen_patch.revert();
    }

    if (characterChangeSeventeen)
    {
        if (toggle)
            characterChangeSeventeen_patch.apply(characterChangeSeventeen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeSeventeen_patch.revert();
    }

    if (characterChangeEighteen)
    {
        if (toggle)
            characterChangeEighteen_patch.apply(characterChangeEighteen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeEighteen_patch.revert();
    }

    if (characterChangeNineteen)
    {
        if (toggle)
            characterChangeNineteen_patch.apply(characterChangeNineteen, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeNineteen_patch.revert();
    }

    if (characterChangeTwenty)
    {
        if (toggle)
            characterChangeTwenty_patch.apply(characterChangeTwenty, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeTwenty_patch.revert();
    }

    if (characterChangeTwentyOne)
    {
        if (toggle)
            characterChangeTwentyOne_patch.apply(characterChangeTwentyOne, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        else
            characterChangeTwentyOne_patch.revert();
    }
}

void hlMain::ToggleSprintFasterActivate(bool toggle)
{
    if (sprintFasterActivate)
    {
        if (toggle)
            sprintFasterActivate_patch.apply(sprintFasterActivate, "\x77\x33", 2);
        else
            sprintFasterActivate_patch.revert();
    }
}

void hlMain::ToggleEnemyAttackOffscreen(bool toggle)
{
    if (enemyAttackOffscreen)
    {
        if (toggle)
            enemyAttackOffscreen_patch.apply(enemyAttackOffscreen, "\x73\x1e", 2);
        else
            enemyAttackOffscreen_patch.revert();
    }
}

void hlMain::ToggleSlowWalk(bool toggle)
{
    if (slowWalkOne)
    {
        if (toggle)
            slowWalkOne_patch.apply(slowWalkOne, "\x0c\x14\x00\x00\x04\x0f\x83", 7);
        else
            slowWalkOne_patch.revert();
    }

    if (slowWalkTwo)
    {
        if (toggle)
            slowWalkTwo_patch.apply(slowWalkTwo, "\x0c\x14\x00\x00\x39\xc8", 6);
        else
            slowWalkTwo_patch.revert();
    }
}

void hlMain::ToggleHideStyle(bool toggle)
{
    if (hideStyle)
    {
        if (toggle)
            hideStyle_patch.apply(hideStyle, "\x0f\x83\x5b\x03\x00\x00", 6);
        if (toggle)
            hideHUDOne_patch.apply(hideHUDOne, "\x74\x2E", 2);
        if (toggle)
            hideHUDTwo_patch.apply(hideHUDTwo, "\x74\x76", 2);
        else
            hideStyle_patch.apply(hideStyle, "\x0f\x84\x5b\x03\x00\x00", 6);
    }

    if (hideOrbs)
    {
        if (toggle)
            hideOrbs_patch.apply(hideOrbs, "\x80\xbf\xfe\x00\x00\x00\xff", 7);
        else
            hideOrbs_patch.apply(hideOrbs, "\x80\xbf\xfe\x00\x00\x00\x00", 7);
    }

    if (hideTimer)
    {
        if (toggle)
            hideTimer_patch.apply(hideTimer, "\xc6\x40\x04\x00", 4);
        else
            hideTimer_patch.apply(hideTimer, "\xc6\x40\x04\x01", 4);
    }
}

void hlMain::ToggleAutoSkipIntro(bool toggle)
{
    if (autoSkipIntro)
    {
        if (toggle)
            autoSkipIntro_patch.apply(autoSkipIntro, "\x80\xb8\x88\x00\x00\x00\x01", 7);
        else
            autoSkipIntro_patch.revert();
    }
}

void hlMain::ToggleAutoSkipOutro(bool toggle)
{
    if (autoSkipOutro)
    {
        if (toggle)
            autoSkipOutro_patch.apply(autoSkipOutro, "\x80\xb8\x88\x00\x00\x00\x01", 7);
        else
            autoSkipOutro_patch.revert();
    }
}

void hlMain::ToggleInfiniteRevive(bool toggle)
{
    if (infiniteReviveOne)
    {
        if (toggle)
            infiniteReviveOne_patch.apply(infiniteReviveOne, "\x88\x9e\xf6\x00\x00\x00", 6);
        else
            infiniteReviveOne_patch.revert();
    }

    if (infiniteReviveTwo)
    {
        if (toggle)
            infiniteReviveTwo_patch.apply(infiniteReviveTwo, "\x72\x0d", 2);
        else
            infiniteReviveTwo_patch.revert();
    }

    if (infiniteReviveThree)
    {
        if (toggle)
            infiniteReviveThree_patch.apply(infiniteReviveThree, "\x83\x80\x3c\x01\x00\x00\x00", 7);
        else
            infiniteReviveThree_patch.revert();
    }

    if (infiniteReviveFour)
    {
        if (toggle)
            infiniteReviveFour_patch.apply(infiniteReviveFour, "\x76\x0d", 2);
        else
            infiniteReviveFour_patch.revert();
    }
}

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

void hlMain::ToggleInfiniteTableHopper(bool toggle)
{
    if (infiniteTableHopper)
    {
        if (toggle)
            infiniteTableHopper_patch.apply(infiniteTableHopper, "\x80\xbe\x14\xce\x00\x00\x00", 7);
        else
            infiniteTableHopper_patch.revert();
    }
}

void hlMain::ToggleInfiniteTrickRange(bool toggle)
{
    if (infiniteTrickRange)
    {
        if (toggle)
            infiniteTrickRange_patch.apply(infiniteTrickRange, "\x90\x90\x90", 3);
        else
            infiniteTrickRange_patch.revert();
    }
}

void hlMain::ToggleCameraSensitivity(bool toggle)
{
    if (cameraSensitivity)
    {
        if (toggle)
            cameraSensitivity_patch.apply(cameraSensitivity, "\x90\x90\x90\x90\x90\x90", 6);
        else
            cameraSensitivity_patch.revert();
    }
}
