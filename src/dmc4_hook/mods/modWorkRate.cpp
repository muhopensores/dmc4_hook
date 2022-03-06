#include "modWorkRate.hpp"
#include "../sdk/Devil4.hpp"

bool WorkRate::disableTrainerPause = false;
float desiredTurboSpeed = 1.2f;
float desiredGlobalSpeed = 1.0f;
float desiredRoomSpeed = 1.0f;
float desiredPlayerSpeed = 1.0f;
float desiredEnemySpeed = 1.0f;

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

// restoring speeds on entering stage would be ideal but this is a fix for how it would reset every load
void WorkRate::onFrame(fmilliseconds& dt) {
    sWorkRate* sWorkRatePtr = Devil4SDK::getWorkRate();
    if (checkWorkRatePtr(sWorkRatePtr)) {
		sWorkRatePtr->turboSpeed = desiredTurboSpeed;
        sWorkRatePtr->globalSpeed = desiredGlobalSpeed;
		sWorkRatePtr->roomSpeed = desiredRoomSpeed;
        sWorkRatePtr->playerSpeed = desiredPlayerSpeed;
		sWorkRatePtr->enemySpeed = desiredEnemySpeed;
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
		ImGui::PushItemWidth(217);
		ImGui::InputFloat("Turbo Speed", &desiredTurboSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
		ImGui::InputFloat("Global Speed", &desiredGlobalSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
		ImGui::InputFloat("Room Speed", &desiredRoomSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
		ImGui::InputFloat("Player Speed", &desiredPlayerSpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
		ImGui::InputFloat("Enemy Speed", &desiredEnemySpeed, 0.1f, 0.5f, "%.1f%");
		ImGui::PopItemWidth();
        if (ImGui::Button("Restore Default Values", ImVec2(217, 20))) {
			desiredTurboSpeed = 1.2f;
			desiredGlobalSpeed = 1.0f;
			desiredRoomSpeed = 1.0f;
			desiredPlayerSpeed = 1.0f;
			desiredEnemySpeed = 1.0f;
		}
		ImGui::Checkbox("Disable Game Pause when opening the trainer", &disableTrainerPause);
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
	else {
		sWorkRatePtr->globalSpeed = desiredGlobalSpeed;
	}
}


void WorkRate::onConfigLoad(const utils::Config & cfg) {
    disableTrainerPause = cfg.get<bool>("disable_trainer_pause").value_or(false);
    desiredTurboSpeed = cfg.get<float>("desired_turbo_speed").value_or(1.2f);
    desiredGlobalSpeed = cfg.get<float>("desired_global_speed").value_or(1.0f);
    desiredRoomSpeed = cfg.get<float>("desired_room_speed").value_or(1.0f);
    desiredPlayerSpeed = cfg.get<float>("desired_player_speed").value_or(1.0f);
    desiredEnemySpeed = cfg.get<float>("desired_enemy_speed").value_or(1.0f);
}

void WorkRate::onConfigSave(utils::Config & cfg) {
    cfg.set<bool>("disable_trainer_pause", disableTrainerPause);
    cfg.set<float>("desired_turbo_speed", desiredTurboSpeed);
    cfg.set<float>("desired_global_speed", desiredGlobalSpeed);
    cfg.set<float>("desired_room_speed", desiredRoomSpeed);
    cfg.set<float>("desired_player_speed", desiredPlayerSpeed);
    cfg.set<float>("desired_enemy_speed", desiredEnemySpeed);
}