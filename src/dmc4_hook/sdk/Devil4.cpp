
#include "Devil4.hpp"

namespace Devil4SDK {

	uPlayer* getLocalPlayer() {
		constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
		static sMediator* sMediatorPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
		return sMediatorPtr->playerPtr;
	}

	sStylishCount* getStylishCount() {
		constexpr uintptr_t sStylishCountPtr = 0x00E558CC;
		static sStylishCount* sc = (sStylishCount*)*(uintptr_t*)sStylishCountPtr;
		return sc;
	}

	sWorkRate* getWorkRate() {
		constexpr uintptr_t sWorkRatePtr = 0x00E558D0;
		static sWorkRate* sw = (sWorkRate*)*(uintptr_t*)sWorkRatePtr;
		return sw;
	}
}