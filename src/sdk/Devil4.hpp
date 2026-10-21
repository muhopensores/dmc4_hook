#pragma once
#include "ReClass.hpp"
#include "sdk/Cam.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/uActor.hpp"
#include "sdk/uPlayer.hpp"
#include <utility>

class sArea;
class sRender;
class sMediator;
class sWorkRate;
class sStylishCount;
class sUnit;
class sResource;

namespace devil4_sdk {
	void u_em003_shl_new();
	sArea* get_sArea();
	sUnit* get_sUnit();
	int get_enemy_count();
	uActor* get_moveline_top(int ml);
	sRender* get_sRender();
	sMediator* get_sMediator();
	uPlayer* get_local_player();
	uCameraCtrl* get_local_camera();
	cCameraPlayer* get_player_camera();
	sStylishCount* get_stylish_count();
	sWorkRate* get_work_rate();
    bool is_paused();
    bool is_not_in_gameplay();
	bool was_gameplay_state_toggled();
    bool is_playing();
	bool is_in_bp();
	std::pair<uint16_t, const char*> getButtonInfo(uint16_t buttonNum);
	float get_current_style_rank();
	bool __stdcall internal_kb_check(uint32_t input);
	MtObject* __stdcall effect_generator(char* efl_path, void* uPlayer, uint8_t op = 0x14,
										uint32_t Group = -1, uint32_t Material = -1, bool isStationary = false,
										float* PosBuffer = nullptr, float* RotBuffer = nullptr);
	void __stdcall effect_cleanup(uintptr_t effect_ptr);
	// A stay effect out of the game's effect table (sDevil4Effect::SetEffectStay 0x4805D0: `list` / `index` into the 0x39
	// path lists at 0xDEB8B8, counts at 0xDEB9A0), placed at `pos` (a 16-byte vector) with orientation `quat` (MtQuaternion
	// x y z w), unit line 0x14, group / material masks -1, `owner` as its owner. With `register_on_owner` the effect is
	// then handed to uPlayer::setEffect(effect, EFCT_TYPE_ERASE_RESET, -1, true, true), as the air hike does, so the
	// player's effect list owns it (erased on a reset, released at once when its effects are off): `owner` must be a
	// uPlayer then. Nero's air hike is list 1 index 0x28 (effect\efl\com\ec000_00v2), Dante's 2 / 0x14 (ec005_20v1).
	// Returns the uDevil4Effect, or null when the list / index is out of range or the .efl didn't load.
	void* __stdcall effect_stay(int list, int index, const float* quat, const float* pos, void* owner, bool register_on_owner = true);
	// SE `id` of a player's own sound resource (uPlayer +0x1D60, mSeCtrl.member_0x160), positioned on the player
	// (sDevil4Sound::requestSe 0xA47AA0 with sSoundN::soundPositionUpdateCb). The air hike plays 40.
	void __stdcall play_player_se(void* player, int id);
    void __stdcall indexed_anim_call(uint32_t id, uPlayer* actor, uint32_t mode = 0,
									float speed = 1.0f, float startFrame = 0.0f,
									float interpolationFrame = 3.0f);
	void __stdcall neutral_air_transition(uPlayer* uActor);
	MtObject* __stdcall get_stuff_from_files(MtDTI* dti, char* path, uint32_t mode = 1);
	void __stdcall release_resource(cResource* rsrc);
	void spawn_or_something(void* a1, MtObject* obj_to_spawn, int a3);
	void sUnit_spawn(void* obj, int move_line);
	void* mt_allocate_heap(size_t size, int alignment);
	void* unit_deallocate(MtObject* obj);
	void* __cdecl MemberFuncToPtr(...);
	void __stdcall bring_assert(void* rFile);
	MtDTI* get_DTI(void* obj);
    bool is_loading_enemy();
    bool is_loading_arc();
    uActor* easy_spawn(uintptr_t spawn_addr, int moveLine);
	bool check_exists_in_moveline(void* actorToFind, int moveLine);
	// Motion banks (uModel::mpMotionListArr[16] at +0xA38): load an .lmt into `bank` of `model` the way
	// uPlayerNero::setBusterMotionRsrc 0x7E7F30 does (sResource::create -> uModel::setMotionList -> release our ref).
	// Returns false when the file didn't load. The model's destructor releases the slot; unload only to swap early.
	bool __stdcall load_motion_bank(void* model, const char* lmt_path, int bank);
	void __stdcall unload_motion_bank(void* model, int bank);
	// .ctn borrow (MoveDef D15): the player's next motion request reads the cancel script of `bank` : `idx` of its
	// rCharTbl for both body halves (uPlayer::mMotionSetCharTblParam +0x2A3C, 12 bytes per half), so a motion of a
	// runtime-loaded bank (which has no script of its own) gets a vanilla one.
	void __stdcall borrow_ctn(void* player, uint32_t bank, uint32_t idx);
	// A .ctn script synthesised in memory (MoveDef D11): `wait(frame)` then one contact command opening every cancel
	// category, then end. cCharTblMgr::update runs it like a vanilla script and updateContactInfoSub opens the windows.
	//   00 mode pad16 f32 frame   wait until mFrame >= frame (mode 0: go on)
	//   01 01 b0 .. b5            contact set 1: byte k's low nibble picks cancelNoTbl[k] (0 = C_NORM0, C_COMMAND, C_JUMP,
	//                             C_CHOUHATSU, C_GUN, C_ALL for k = 0..5), high nibble 0x10 = open now, 0x30 = reserve
	//   FF                        end
#pragma pack(push, 1)
	struct CancelScript {
		uint8_t  wait_op = 0, wait_mode = 0; uint16_t wait_pad = 0; float wait_frame = 0.0f;
		uint8_t  contact_op = 1, contact_idx = 1, info[6] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
		uint8_t  end_op = 0xFF, pad[3] = { 0, 0, 0 };
	};
#pragma pack(pop)
	static_assert(sizeof(CancelScript) == 20, "ctn script layout");
	// Points both halves' cCharTblMgr cursors (uPlayer +0x1DD4, 0x24 per half: +0xC mpDataTop, +0x10 mpDataCrnt, +0x14
	// mIsDone) at `script` with its wait set to `frame`. The script must outlive the motion (keep it static, one per
	// player). Call on the tick after the motion request: updateMotion installs the borrowed script for the new motion
	// first and would overwrite the cursors.
	void __stdcall install_cancel_script(void* player, CancelScript& script, float frame);
	uintptr_t __stdcall get_motion_bank(void* model, int bank);   // the rMotionList* in that slot, 0 = empty
};

namespace uactor_sdk {
	void __stdcall load_atk_col(void* rAtck, void* rCol, void* ColMgr, void* Obj);
	void __stdcall get_model(void* obj, void* model);
	void __stdcall updateLmat(void* obj);
	void __stdcall updateWmat(void* obj);
	void __stdcall despawn(void* obj);
	void __stdcall ushell_des(void* obj);
	void __stdcall uActorCons(void* obj);
	void __stdcall uCollisionMgrCons(void* obj);
	void __stdcall collide(void* CollMgr);
	void __stdcall hitbox_call(void* CollMgr, int id);
	void __stdcall render_call(void* render_obj, void* trans);
	void __stdcall uDevil4ModelCons(void* obj);
	void __stdcall uDevil4ModelDest(void* obj);
	void __stdcall destructor_call(void* obj);
	void* __stdcall get_joint_from_index(void* obj, int index);
	void* __stdcall get_center_pos(void* obj, Vector3f* pos);
}
