#include "WorkRate.hpp"
#include "../sdk/Devil4.hpp"

bool WorkRate::disable_trainer_pause = false;
static bool force_custom_turbo = false;
static float custom_turbo = 1.2f;
static bool hotkey_paused = false;
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
		mov [eax+0x20],ecx
		mov [eax+0x24],ecx
		jmp DWORD PTR [WorkRate::jmp_return]
	}
}
#endif

std::optional<std::string> WorkRate::on_initialize() {

	/*if (!install_hook_offset(0xA948, hook, &detour, &jmp_return, 6)) {
		return "Failed to init WorkRate mod";
	}*/

    utility::create_keyboard_hotkey(m_hotkeys, {VK_DECIMAL}, "Pause Game", "pause_game");

	console->system().RegisterCommand("turbo", "Set turbo speed for current room", 
        [](float value) {
			if (sWorkRate* workRatePtr = devil4_sdk::get_work_rate()) {
				workRatePtr->turbo_speed = value;
			}
        }, 
        csys::Arg<float>("1.2 default"));

	return Mod::on_initialize();
}

inline bool WorkRate::check_work_rate_ptr(sWorkRate* wr) {
	if (IsBadWritePtr(wr, sizeof(uint32_t))) {
		return false;
	}
	return true;
}

void WorkRate::on_gui_frame() {
	if (ImGui::CollapsingHeader(_("Speed"))) {
		sWorkRate* s_work_rate_ptr = devil4_sdk::get_work_rate();
		if (!check_work_rate_ptr(s_work_rate_ptr)) {
			ImGui::TextWrapped(_("Speed adjustments are not initialized yet, load into the stage to access them."));
			ImGui::Spacing();
			return;
		}
		ImGui::PushItemWidth(sameLineItemWidth);
		ImGui::InputFloat(_("Turbo Value"), &s_work_rate_ptr->turbo_speed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
        ImGui::InputFloat(_("Game Speed"), &s_work_rate_ptr->game_speed, 0.1f, 0.5f, "%.1f%");
        ImGui::SameLine();
        help_marker(_("Enemies, players, room, bullets, pins, camera"));
		ImGui::Spacing();
        ImGui::InputFloat(_("Global Speed"), &m_global_speed, 0.1f, 0.5f, "%.1f%");
		ImGui::SameLine();
        help_marker(_("Enemies, players, room"));
		ImGui::Spacing();
        ImGui::InputFloat(_("Room Speed"), &s_work_rate_ptr->room_speed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
        ImGui::InputFloat(_("Player Speed"), &s_work_rate_ptr->player_speed, 0.1f, 0.5f, "%.1f%");
		ImGui::Spacing();
        ImGui::InputFloat(_("Enemy Speed"), &s_work_rate_ptr->enemy_speed, 0.1f, 0.5f, "%.1f%");
		ImGui::PopItemWidth();
		ImGui::Checkbox(_("Disable Game Pause when opening the trainer"), &disable_trainer_pause);
        ImGui::Checkbox(_("Force Custom Turbo"), &force_custom_turbo);
        ImGui::SameLine();
        help_marker(_("This turbo won't be disabled on room change"));
		if (force_custom_turbo) {
			ImGui::Indent(lineIndent);
			ImGui::PushItemWidth(sameLineItemWidth);
			ImGui::InputFloat(_("Custom Turbo"), &custom_turbo, 0.1f, 0.5f, "%.1f%");
			ImGui::PopItemWidth();
			ImGui::Unindent(lineIndent);
		}
	}
}

void WorkRate::on_frame(fmilliseconds& dt) {
	if (force_custom_turbo) {
		sWorkRate* s_work_rate_ptr = devil4_sdk::get_work_rate();
		s_work_rate_ptr->turbo_speed = custom_turbo;
	}
}

void WorkRate::on_game_pause(bool toggle) {
	sWorkRate* s_work_rate_ptr = devil4_sdk::get_work_rate();
	if (!check_work_rate_ptr(s_work_rate_ptr)) {
		return;
	}
	if (toggle == true && disable_trainer_pause == false) {
		s_work_rate_ptr->global_speed = 0.0f;
	}
	if (toggle == false) {
		if (!hotkey_paused)
			s_work_rate_ptr->global_speed = m_global_speed;
	}
}

void WorkRate::on_update_input(utility::Input& input) {
    sWorkRate* s_work_rate_ptr = devil4_sdk::get_work_rate();
    if (!check_work_rate_ptr(s_work_rate_ptr)) {
        return;
    }
    if (m_hotkeys[0]->check(input)) {
        if (hotkey_paused){
			s_work_rate_ptr->global_speed = m_global_speed;
			hotkey_paused = false;
        } else {
			s_work_rate_ptr->global_speed = 0.0f;
			hotkey_paused = true;
        }
    }
}

void WorkRate::on_config_load(const utility::Config & cfg) {
    disable_trainer_pause = cfg.get<bool>("disable_trainer_pause").value_or(false);
    force_custom_turbo = cfg.get<bool>("force_custom_turbo").value_or(false);
    custom_turbo = cfg.get<float>("custom_turbo").value_or(1.2f);
}

void WorkRate::on_config_save(utility::Config & cfg) {
    cfg.set<bool>("disable_trainer_pause", disable_trainer_pause);
    cfg.set<bool>("force_custom_turbo", force_custom_turbo);
    cfg.set<float>("custom_turbo", custom_turbo);
}
