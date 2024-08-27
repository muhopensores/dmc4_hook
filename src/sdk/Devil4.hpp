#pragma once
#include "ReClass.hpp"

namespace devil4_sdk {
	void u_em003_shl_new();
	void dog_fireball_new();
	sArea* get_sArea();
	SMediator* get_sMediator();
	uPlayer* get_local_player();
	uCameraCtrl* get_local_camera();
	cCameraPlayer* get_player_camera();
	SStylishCount* get_stylish_count();
	sWorkRate* get_work_rate();
    bool is_paused();
    bool is_not_in_gameplay();
	bool was_gameplay_state_toggled();
	std::pair<uint16_t, const char*> getButtonInfo(uint16_t buttonNum);
	float get_current_style_rank();
	bool __stdcall internal_kb_check(uint32_t input);
	MtObject* __stdcall effect_generator(char* efl_path, void* uPlayer, uint8_t op = 0x14,
										uint32_t Group = -1, uint32_t Material = -1, bool isStationary = false,
										float* PosBuffer = nullptr, float* RotBuffer = nullptr);
	void __stdcall effect_cleanup(uintptr_t effect_ptr);
    void __stdcall indexed_anim_call(uint32_t id, uPlayer* actor, uint32_t mode = 0,
									float speed = 1.0f, float startFrame = 0.0f,
									float interpolationFrame = 3.0f);
	void __stdcall neutral_air_transition(uPlayer* uActor);
	MtObject* __stdcall get_stuff_from_files(MtDTI* dti, char* path, uint32_t mode = 1);
	void __stdcall release_resource(CResource* rsrc);
};
