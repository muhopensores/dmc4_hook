#include "naked.h"
#include "aobs.h"
#include "addresses.h"

// hooks and jumps to get back to the correct address after hooking
void HookJumps() {

	if (damagemodifier != 0)
	{
		auto damagemodifier_hk = m_hook.hookJMP(damagemodifier, 5, &damagemodifier_proc, &_damagemodifierContinue);
	}

	if (infiniteDT != 0)
	{
		auto infiniteDT_hk = m_hook.hookJMP(infiniteDT, 8, &infiniteDT_proc, &_infiniteDTContinue);
	}

	if (infinitePlayerHealth != 0)
	{
		auto infinitePlayerHealth_hk =
			m_hook.hookJMP(infinitePlayerHealth, 8, &infinitePlayerHealth_proc, &_infinitePlayerHealthContinue);
	}

	if (berialDazeTwo != 0)
	{
		auto berialDazeTwo_hk = m_hook.hookJMP(berialDazeTwo, 8, &berialDaze_proc, &_berialDazeContinue);
	}

	if (moveIDAlloc != 0)
	{
		auto moveIDAlloc_hk = m_hook.hookJMP(moveIDAlloc, 6, &moveIDAlloc_proc);
	}

	if (selectiveCancels != 0)
	{
		auto selectiveCancels_hk = m_hook.hookJMP(selectiveCancels, 6, &selectiveCancels_proc);
	}

	if (cameraHeightSetting != 0)
	{
		auto cameraHeightSetting_hk = m_hook.hookJMP(cameraHeightSetting, 8, &cameraHeight_proc, &_cameraHeightContinue);
	}

	if (cameraDistanceSetting != 0)
	{
		auto cameraDistanceSetting_hk =
			m_hook.hookJMP(cameraDistanceSetting, 8, &cameraDistance_proc, &_cameraDistanceContinue);
	}

	if (cameraDistanceLockonSetting != 0)
	{
		auto cameraDistanceLockonSetting_hk =
			m_hook.hookJMP(cameraDistanceLockonSetting, 8, &cameraDistanceLockon_proc, &_cameraDistanceLockonContinue);
	}

	if (cameraAngleSetting != 0)
	{
		auto cameraAngleSeting_hk = m_hook.hookJMP(cameraAngleSetting, 8, &cameraAngle_proc, &_cameraAngleContinue);
	}

	if (ldkWithDMDOne != 0)
	{
		auto ldkWithDMDOne_hk = m_hook.hookJMP(ldkWithDMDOne, 6, &ldkWithDMD_proc); //, &_ldkWithDMDContinue);
	}

	if (lockOn != 0)
	{
		auto lockOn_hk = m_hook.hookJMP(lockOn, 10, &lockOn_proc, &_lockOnContinue);
	}

	if (lockOff != 0)
	{
		auto lockOff_hk = m_hook.hookJMP(lockOff, 10, &lockOff_proc, &_lockOffContinue);
	}

	if (trackingFullHouse != 0)
	{
		auto trackingFullHouse_hk = m_hook.hookJMP(trackingFullHouse, 7, &trackingFullHouse_proc);
	}

	if (orbDisplay != 0)
	{
		auto orbDisplay_hk = m_hook.hookJMP(orbDisplay, 6, &orbDisplay_proc); //, &_orbDisplayContinue
	}

	if (timerAlloc != 0)
	{
		auto timerAlloc_hk = m_hook.hookJMP(timerAlloc, 10, &timerAlloc_proc);
	}

	if (backForward != 0)
	{
		auto backForward_hk = m_hook.hookJMP(backForward, 8, &backForward_proc);
	}

	if (trickDown != 0)
	{
		auto trickDown_hk = m_hook.hookJMP(trickDown, 8, &trickDown_proc);
	}

	if (floorTouch != 0)
	{
		auto floorTouch_hk = m_hook.hookJMP(floorTouch, 8, &floorTouch_proc);
	}

	if (roseRemovesPins != 0)
	{
		auto roseRemovesPins_hk = m_hook.hookJMP(roseRemovesPins, 10, &roseRemovesPins_proc);
	}
}
