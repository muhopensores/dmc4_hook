#include "modWorkRate.hpp"
#include "../sdk/Devil4.hpp"

bool WorkRate::disableTrainerPause = false;
static bool forceCustomTurbo = false;
static float customTurbo = 1.2f;
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

inline bool WorkRate::checkWorkRatePtr(sWorkRate* wr) {
	if (IsBadWritePtr(wr, sizeof(uint32_t))) {
		return false;
	}
	else {
		return true;
	}
}

void WorkRate::onGUIframe() {
	if (ImGui::CollapsingHeader("Speed")) {
		sWorkRate* sWorkRatePtr = Devil4SDK::getWorkRate();
		if (!checkWorkRatePtr(sWorkRatePtr)) {
			ImGui::TextWrapped("Speed adjustments are not initialized yet, load into the stage to access them.");
			ImGui::Spacing();
			return;
		}
		ImGui::PushItemWidth(224);
		ImGui::InputFloat("Turbo Value", &sWorkRatePtr->turboSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
        ImGui::InputFloat("Game Speed", &sWorkRatePtr->gameSpeed, 0.1f, 0.5f, "%.1f%");
        ImGui::SameLine();
        HelpMarker("Enemies, players, room, bullets, pins, camera");
		ImGui::Spacing();
        ImGui::InputFloat("Global Speed", &m_globalSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::SameLine();
        HelpMarker("Enemies, players, room");
		ImGui::Spacing();
        ImGui::InputFloat("Room Speed", &sWorkRatePtr->roomSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
        ImGui::InputFloat("Player Speed", &sWorkRatePtr->playerSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
        ImGui::InputFloat("Enemy Speed", &sWorkRatePtr->enemySpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::PopItemWidth();
		ImGui::Checkbox("Disable Game Pause when opening the trainer", &disableTrainerPause);
        ImGui::Checkbox("Force Custom Turbo", &forceCustomTurbo);
        ImGui::SameLine();
        HelpMarker("This turbo won't be reverted on room change");
		if (forceCustomTurbo) {
			ImGui::PushItemWidth(224);
			ImGui::InputFloat("Custom Turbo", &customTurbo, 0.1f, 0.5f, "%.1f%");
			ImGui::PopItemWidth();
		}
	}
}

void WorkRate::onFrame(fmilliseconds& dt) {
	if (forceCustomTurbo) {
		sWorkRate* sWorkRatePtr = Devil4SDK::getWorkRate();
		sWorkRatePtr->turboSpeed = customTurbo;
	}
}

void WorkRate::onGamePause(bool toggle) {
	sWorkRate* sWorkRatePtr = Devil4SDK::getWorkRate();
	if (!checkWorkRatePtr(sWorkRatePtr)) {
		return;
	}
	if (toggle == true && disableTrainerPause == false) {
		sWorkRatePtr->globalSpeed = 0.0f;
	}
	if (toggle == false) {
		sWorkRatePtr->globalSpeed = m_globalSpeed;
	}
}

void WorkRate::onConfigLoad(const utils::Config & cfg) {
    disableTrainerPause = cfg.get<bool>("disable_trainer_pause").value_or(false);
    forceCustomTurbo = cfg.get<bool>("force_custom_turbo").value_or(false);
    customTurbo = cfg.get<float>("custom_turbo").value_or(1.2f);
}

void WorkRate::onConfigSave(utils::Config & cfg) {
    cfg.set<bool>("disable_trainer_pause", disableTrainerPause);
    cfg.set<bool>("force_custom_turbo", forceCustomTurbo);
    cfg.set<float>("custom_turbo", customTurbo);
}
