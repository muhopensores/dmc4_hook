#include "modWorkRate.hpp"
#include "../sdk/Devil4.hpp"


#if 0
uintptr_t  WorkRate::jmp_return{ NULL };
sWorkRate* WorkRate::sWorkRatePtr{ NULL };
WorkRate::WorkRate() {
	//onInitialize();
}

naked void detour() {
	//DevilMayCry4_DX9.exe+A948 - 89 48 20              - mov [eax+20],ecx
	__asm {
		mov DWORD PTR [WorkRate::sWorkRatePtr], eax
		mov [eax+20h],ecx
		mov [eax+24h],ecx
		jmp DWORD PTR [WorkRate::jmp_return]
	}
}
#endif

std::optional<std::string> WorkRate::onInitialize() {

	/*if (!install_hook_offset(0xA948, hook, &detour, &jmp_return, 6)) {
		return "Failed to init WorkRate mod";
	}*/

	return Mod::onInitialize();
}

/*void WorkRate::onFrame(fmilliseconds& dt) {

}*/

inline bool WorkRate::checkWorkRatePtr(sWorkRate* wr) {
	if (IsBadWritePtr(wr, sizeof(uint32_t))) {
		return false;
	}
	else {
		return true;
	}
}

void WorkRate::onConfigLoad(const utils::Config & cfg) {

}

void WorkRate::onConfigSave(utils::Config & cfg) {

}

void WorkRate::onGUIframe() {
		if (ImGui::CollapsingHeader("Speed"))
		{
			sWorkRate* sWorkRatePtr = Devil4SDK::getWorkRate();
			if (!checkWorkRatePtr(sWorkRatePtr)) {
				ImGui::TextWrapped("Speed adjustments are not initialized yet, load into the stage to access them.");
				ImGui::Spacing();
				return;
			}
				ImGui::InputFloat("Turbo Value", &sWorkRatePtr->turboSpeed, 0.1f, 0.5f, "%.1f%");
				ImGui::Spacing();
				ImGui::SliderFloat("Global Speed", &m_globalSpeed, 0.0f, 3.0f, "%.1f");
				ImGui::Spacing();
				ImGui::SliderFloat("Room Speed", &sWorkRatePtr->roomSpeed, 0.0f, 3.0f, "%.1f");
				ImGui::Spacing();
				ImGui::SliderFloat("Player Speed", &sWorkRatePtr->playerSpeed, 0.0f, 3.0f, "%.1f");
				ImGui::Spacing();
				ImGui::SliderFloat("Enemy Speed", &sWorkRatePtr->enemySpeed, 0.0f, 3.0f, "%.1f");
		}
}
void WorkRate::onGamePause(bool toggle) {

	sWorkRate* sWorkRatePtr = Devil4SDK::getWorkRate();
	if (!checkWorkRatePtr(sWorkRatePtr)) {
		return;
	}
	if (toggle) {
		sWorkRatePtr->globalSpeed = 0.0f;
	}
	else {
		sWorkRatePtr->globalSpeed = m_globalSpeed;
	}
}