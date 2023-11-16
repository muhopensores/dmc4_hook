#pragma once
#include "ReClass.hpp"

namespace devil4_sdk {
	void u_em003_shl_new();
	uPlayer* get_local_player();
	uCameraCtrl* get_local_camera();
	cCameraPlayer* get_player_camera();
	SStylishCount* get_stylish_count();
	sWorkRate* get_work_rate();
    bool is_paused();
    bool is_not_in_gameplay();
	bool was_gameplay_state_toggled();
	float get_current_style_rank();
};