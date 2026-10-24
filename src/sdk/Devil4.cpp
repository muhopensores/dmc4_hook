#include "sdk/Devil4.hpp"
#include <spdlog/spdlog.h>
#include "sdk/StringData.hpp"
#include "sdk/aGame.hpp"
#include "sdk/sArea.hpp"
#include "sdk/sRender.hpp"
#include "sdk/sWorkRate.hpp"
#include "sdk/sMediator.hpp"
#include "sdk/MtAllocator.hpp"
#include "sdk/sStylishCount.hpp"
#include "sdk/uEm003.hpp"
#include "sdk/sUnit.hpp"
#include "Windows.h"

static MtHeapAllocator** mt_heap_alloc_static_ptr = (MtHeapAllocator**)0x00E1434C;

static uintptr_t fptr_update_actor_list{ 0x008DC540 }; // Spawns shit
static constexpr uintptr_t sUnit_static_ptr { 0x00E552CC };
static uintptr_t sKeyboard{ 0x00E559C0 };
static uintptr_t sDevil4Resource { 0x00E552D0 };

namespace devil4_sdk {
	void* mt_allocate_heap(size_t size, int alignment) {
		static MtHeapAllocator* mt_heap_alloc = *mt_heap_alloc_static_ptr;

		return (void*)mt_heap_alloc->allocate(size, alignment);
	}
    void* unit_deallocate(MtObject* obj) {
        static MtHeapAllocator* unit_heap_alloc = *mt_heap_alloc_static_ptr;

        return (void*)unit_heap_alloc->deallocate(obj);
    }

	MtDTI* get_DTI(void* obj) {
        uintptr_t dti_call = *(uintptr_t*)(*(uintptr_t*)obj + 0x10);
        return ((MtDTI * (*)())(dti_call))();
    }

	bool is_loading_enemy() {
        sArea* area = devil4_sdk::get_sArea();
        if (!area) {
            return true;
        }
        aRoom* room = area->aRoomPtr;
        if (!room) {
            return true;
        }
        __asm {
			mov edi, room
			mov edx, 0x40EBC0
			call edx
        }
    }

	// check room is created and is_loading_enemy()
	bool is_loading_arc() {
        sArea* area = devil4_sdk::get_sArea();
        if (!area) {
            return true;
        }
        aRoom* room = area->aRoomPtr;
        if (!room) {
            return true;
        }
        if (//room->busy != 0 || 
			is_loading_enemy()
		){
            return true;
        }
        return false;
	}

	uActor* easy_spawn(uintptr_t spawn_addr, int moveLine) {
		__asm {
			mov eax, spawn_addr
			call eax
			mov esi, eax
			mov eax, [0x00E552CC] // sUnit::mpInstance (sUnit_ptr)
			mov eax, [eax]
			mov ecx, moveLine
			push ecx
			mov edx, 0x008DC540 // sUnit::addBottom (fptr_update_actor_list)
			call edx
			mov eax,esi
		}
	}

	//_DWORD *__usercall uEm003Shl_ConstructorMaybe_sub_560330@<eax>(int a1@<eax>)
	MtObject* u_em003_shl_consturctor_sub(void* mem) {
		MtObject* ret_val = 0;
		constexpr uintptr_t fptr_em003_shl_consturctor = 0x00560330;
		__asm {
			mov eax, mem
			call fptr_em003_shl_consturctor
			mov [ret_val], eax
		}
		return ret_val;
	}

	//char __userpurge spawnOrSomething_sub_8DC540@<al>(int sUnitMaybe@<eax>, _DWORD *mtObjectToSpawn@<esi>, int someIndex)
	void spawn_or_something(void* a1, MtObject* obj_to_spawn, int a3) {
		constexpr uintptr_t fptr_spawn_or_something = 0x008DC540;

		/*__asm {
			mov ecx, 12h
			mov eax, [someStruct] // static
			mov eax, [eax]
			mov esi, objToSpawn
			push 12h
			call fptrUpdateActorList
		}*/

		__asm {
			mov eax, [a1]
			mov eax, [eax]
			mov esi, obj_to_spawn
			mov ecx, a3
			push a3
			call fptr_spawn_or_something
		}

	}

	void sUnit_spawn(void* obj, int move_line) {
        constexpr uintptr_t spawn_call = 0x008DC540;
        uintptr_t s_unit_ptr           = (uintptr_t)get_sUnit();
        _asm {
				pushad
				mov eax, s_unit_ptr
				mov esi, obj
				push move_line
				call spawn_call
				popad
        }
     }

	void u_em003_shl_new() {
		void* mem = mt_allocate_heap(sizeof(uEm003Shl), 0x10);
		if (!mem) {
			return;
		}
		
		uEm003Shl* shell = (uEm003Shl*)u_em003_shl_consturctor_sub(mem);
		if (!shell) {
			return;
		}

		spawn_or_something((void*)0x00E552CC, shell, 0x12);

		shell->m_some_float_vec = glm::vec3(0.0f, 0.0f, 0.0f);
		
		auto lp = devil4_sdk::get_local_player();
		shell->m_pos_maybe = lp->mPos;

		shell->m_required_idk_wtf_is_this_needs_to_be2 = 2;
		shell->m_required_idk_wtf_is_this_needs_to_be6 = 0;
		shell->m_required_some_byte_idk = 0;
	}

	sArea* get_sArea() {
		constexpr uintptr_t static_area_ptr = 0x00E552C8;
		sArea* s_area_ptr = (sArea*)*(uintptr_t*)static_area_ptr;
		return s_area_ptr;
	}

	sUnit* get_sUnit() {
		sUnit* s_unit_ptr = (sUnit*)*(uintptr_t*)sUnit_static_ptr;
		return s_unit_ptr;
	}

	// return true if already exists
	bool check_exists_in_moveline(void* actorToFind, int moveLine) {
        if (!actorToFind) { return false; }
        sUnit* s_unit_ptr = get_sUnit();
        if (!s_unit_ptr) { return false; }
        cUnit* unit = (cUnit*)s_unit_ptr->mMoveLine[moveLine].mTop;
        while (unit) {
            if (unit == actorToFind) {
                return true;
            }
            unit = unit->mp_next_unit;
        }
        return false;
    }

	uActor* get_moveline_top(int ml) {
		sUnit* s_unit_ptr = get_sUnit();
		if (s_unit_ptr) {
			uActor* top = (uActor*)s_unit_ptr->mMoveLine[ml].mTop;
			return top;
		}
		else return nullptr;
	}

	int get_enemy_count() {
		sUnit* s_unit_ptr = devil4_sdk::get_sUnit();
		if (!s_unit_ptr) {
			return 0;
		}

		uEnemy_Old* currentEnemy = (uEnemy_Old*)s_unit_ptr->mMoveLine[15].mTop;
		if (!currentEnemy) {
			return 0;
		}
    
		int enemyCount = 0;
    
		while (currentEnemy) {
			enemyCount++;
			currentEnemy = currentEnemy->nextEnemy;
		}
    
		return enemyCount;
	}

	sRender* get_sRender() {
		constexpr uintptr_t static_render_ptr = 0x00E552D8;
		sRender* s_render_ptr = (sRender*)*(uintptr_t*)static_render_ptr;
		return s_render_ptr;
	}	

	sMediator* get_sMediator() {
		constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
		sMediator* s_mediator_ptr = (sMediator*)*(uintptr_t*)static_mediator_ptr;
		return s_mediator_ptr;
	}

	uPlayer* get_local_player() {
		constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
		sMediator* s_mediator_ptr = (sMediator*)*(uintptr_t*)static_mediator_ptr;
		if (!s_mediator_ptr) return nullptr;
        if (!s_mediator_ptr->pad_0) return nullptr;
		return s_mediator_ptr->player_ptr;
	}

	uCameraCtrl* get_local_camera() {
        constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
        sMediator* s_mediator_ptr        = (sMediator*)*(uintptr_t*)static_mediator_ptr;
        if (!s_mediator_ptr) return nullptr;
        if (!s_mediator_ptr->pad_0) return nullptr;
        return s_mediator_ptr->camera1;
    }

    cCameraPlayer* get_player_camera() {
        constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
        sMediator* s_mediator_ptr        = (sMediator*)*(uintptr_t*)static_mediator_ptr;
        if (!s_mediator_ptr || !s_mediator_ptr->camera1) return nullptr;
        if (!s_mediator_ptr->pad_0) return nullptr;
        return s_mediator_ptr->camera1->mpCamPlayer;
    }

	sStylishCount* get_stylish_count() {
		constexpr uintptr_t s_stylish_count_ptr = 0x00E558CC;
		static sStylishCount* sc = (sStylishCount*)*(uintptr_t*)s_stylish_count_ptr;
		return sc;
	}

	float get_current_style_rank() {
		sStylishCount* sc = get_stylish_count();
		if (!sc) { return 1.0f; }

		uint32_t rank = sc->current_style_tier;
		//float normalizedRank = glm::smoothstep(0.0f, 7.0f, (float)(rank+1)); // no style no head
		//                                    edge0 edge1        x
		// smoothstep Returns 0.0 if x <= edge0 and 1.0 if x >= edge1 and performs smooth
		// Hermite interpolation between 0 and 1 when edge0 < x < edge1.
		//return normalizedRank;
		return (float)(rank); // + 1
	}

	sWorkRate* get_work_rate() {
		constexpr uintptr_t s_work_rate_ptr = 0x00E558D0;
		static sWorkRate* sw = (sWorkRate*)*(uintptr_t*)s_work_rate_ptr;
		return sw;
	}

    bool is_paused() {
        if (auto sArea = devil4_sdk::get_sArea()) {
            return sArea->aGamePtr && sArea->aGamePtr->m_paused;
        }
		else
			return false;
    }

    bool is_not_in_gameplay() {
        return devil4_sdk::is_paused() || (!devil4_sdk::get_local_player());
    }

	bool was_gameplay_state_toggled() {
		static bool state = false;
		bool curr_state = is_not_in_gameplay();
		bool ret = curr_state != state;
		state = curr_state;
		return ret;
	}

	// does player exist and have > 0 hp
	bool is_playing() {
		sMediator* sMed = devil4_sdk::get_sMediator();
		if (!sMed) return false;
		uPlayer* player = sMed->player_ptr;
		if (!player) return false;
		return (player->damageStruct.HP > 0.0f);
	}

	bool is_in_bp() {
		sMediator* sMed = devil4_sdk::get_sMediator();
		if (!sMed) return false;
		return (sMed->missionID == 50);
	}

	std::pair<uint16_t, const char*> getButtonInfo(uint16_t buttonNum) {
        for (const auto& pair : buttonPairs) {
			if (pair.first == buttonNum) {
            return pair;
			}
        }
            return buttonPairs[0];
    }

	//https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	bool __stdcall internal_kb_check(uint32_t input) {
        _asm{
                mov eax,[sKeyboard]
				mov eax,[eax]
                lea esi,[eax+0x120]
                mov ecx,input //KB scan code
                mov edx,ecx
                and ecx,0x1F
                mov ebx,0x00000001
                shr edx,0x05
                shl ebx,cl
                lea edx,[esi+edx*4]
				mov ecx,[edx]
				and ecx,ebx
				xor eax,eax
				cmp ecx,ebx
				sete al
        }	
	}
		
	MtObject* __stdcall read_efl_file(char* efl_path) {
        constexpr uintptr_t file_read_call = 0x008DF530;
		constexpr uintptr_t eflMtDTI = 0x00EADE88;
		_asm {
				//get efl file
				push 01
				push efl_path
				push eflMtDTI
				mov eax,sDevil4Resource
				mov eax,[eax]
				call file_read_call
		}
	}

	MtObject* __stdcall uDevil4Effect_constructor(void* MtObject) {
        constexpr uintptr_t efct_init = 0x521890;
		_asm {
				mov eax,MtObject
				call efct_init
		}
	}

	void __stdcall TrackingEfxInit(MtObject* sEffect, MtObject* rEffectL, void* uActor, uint32_t Group, uint32_t Material) {
        float dummy1 = 0.0f;
        float dummy2 = 1.0f;
        constexpr uintptr_t call1 = 0x0096A0A0;
        constexpr uintptr_t call2 = 0x008DDA00;
        char reserved[100];
        char reserved2[100];
		_asm {
				mov eax, uActor
				push eax
				//Position buffer
				lea ecx,[reserved]
				lea edi,reserved
				fld dummy1
				fstp [edi]
				fld dummy1
				fstp [edi + 0x4]
				fld dummy1
				fstp [edi + 0x8]
				push ecx
				//Rotation buffer
				lea edi,reserved2
				fld dummy1
				fstp [edi]
				fld dummy1
				fstp [edi + 0x4]
				fld dummy1
				fstp [edi + 0x8]
				fld dummy2
				fstp [edi + 0xC]

				push Material
				push Group
				//push rEffectList
				mov ebx,rEffectL
				push ebx
				//push uDevil4Effect
				mov esi,sEffect
				push esi

				mov eax,0x4
				lea edi,[reserved2]
				or ecx,-1

				call call1

				mov ecx,uActor
				mov al,01
				mov byte ptr [esi+0x1D4],al
				mov byte ptr [esi+0x1D5],al
				mov eax,[sDevil4Resource]
				mov eax,[eax]
				mov edi,ebx
				mov [esi+0x1D0],ecx
				call call2
				mov edx,[esi+0xE0]
				and edx, 0xFFFFFF7F
				or edx,0x800
				mov [esi+0xE0],edx
		}
	}

	void __stdcall StationaryEfxInit(MtObject* sEffect, MtObject* rEffectL, void* uActor, uint32_t Group, uint32_t Material, float* PosBuffer, float* RotBuffer) {
		uintptr_t call1 = 0x0096A100;
		uintptr_t call2 = 0x008DDA00;
		_asm {
			mov edx,RotBuffer
			mov eax,Material
			mov ecx,Group
			mov edi,PosBuffer
			
			push edx
			push eax
			push ecx
			push rEffectL
			mov eax,4
			mov ecx,sEffect

			mov ecx,uActor
			mov byte ptr [esi+0x1D4],1
			mov byte ptr [esi+0x1D5],1
			mov eax,[sDevil4Resource]
			mov eax,[eax]
			mov edi,rEffectL
			mov [esi+0x1D0],ecx
			call call2
		}
	}

	MtObject* __stdcall effect_generator(char* efl_path, void* uPlayer, uint8_t op, uint32_t Group, uint32_t Material, bool isStationary,
            float* PosBuffer, float* RotBuffer) {
		MtObject* eflObject = read_efl_file(efl_path);
        MtObject* uDevil4Effect = uDevil4Effect_constructor(mt_allocate_heap(0x200, 0x10));
        spawn_or_something((void*)0x00E552CC, uDevil4Effect, op);
        if (!isStationary)
			TrackingEfxInit(uDevil4Effect, eflObject, uPlayer, Group, Material);
		else
            StationaryEfxInit(uDevil4Effect, eflObject, uPlayer, Group, Material, PosBuffer, RotBuffer);
		return uDevil4Effect;
	}

	void __stdcall effect_cleanup(uintptr_t effect_ptr) {
            uintptr_t methods = *(uintptr_t*)effect_ptr;
            uintptr_t func = *(uintptr_t*)(methods + 0x30); // effect culling
            void*(__fastcall * pFunc)(uintptr_t a) = (void*(__fastcall*)(uintptr_t))func;
            pFunc((uintptr_t)effect_ptr);
    }

	void* __stdcall effect_stay(int list, int index, const float* quat, const float* pos, void* owner, bool register_on_owner) {
		uintptr_t fn_SetEffectStay = 0x004805D0;   // sDevil4Effect::SetEffectStay(list@eax, idx@ecx, quat*, pos*, group, material, line, owner) -> effect@eax, retn 18h
		uintptr_t fn_setEffect     = 0x0081FD60;   // uPlayer::setEffect(effect@eax, this@ecx, EFCT_TYPE, id, bool, bool) userpurge
		void* effect = nullptr;
		_asm {
			push ebx
			mov ebx, esp
			pushad
			push owner              // owner
			push 0x14               // unit line
			push 0xFFFFFFFF         // material mask
			push 0xFFFFFFFF         // group mask
			push pos
			push quat
			mov eax, list
			mov ecx, index
			call fn_SetEffectStay
			mov effect, eax
			test eax, eax
			jz effect_stay_done
			cmp byte ptr register_on_owner, 0
			je effect_stay_done
			mov ecx, owner          // uPlayer::setEffect(effect, this, EFCT_TYPE_ERASE_RESET = 1, -1, true, true)
			push 1
			push 1
			push 0xFFFFFFFF
			push 1
			call fn_setEffect
		effect_stay_done:
			popad
			mov esp, ebx
			pop ebx
		}
		return effect;
	}

	void* __stdcall effect_const(int list, int index, void* parent, uint32_t group, uint32_t material, int unit_line, void* owner) {
		uintptr_t fn = 0x00480480;   // sDevil4Effect::setEffectConst
		void* effect = nullptr;
		_asm {
			pushad
			push owner
			push unit_line
			push material
			push group
			push parent
			mov eax, list
			mov edx, index
			call fn                  // retn 14h: pops the five
			mov effect, eax
			popad
		}
		return effect;
	}

	void __stdcall register_effect(void* effect, void* player, int efx_type, int id) {
		uintptr_t fn = 0x0081FD60;   // uPlayer::setEffect, userpurge (callee pops the four)
		if (!effect || !player) return;
		_asm {
			push ebx
			mov ebx, esp
			pushad
			mov eax, effect
			mov ecx, player
			push 1
			push 1
			push id
			push efx_type
			call fn
			popad
			mov esp, ebx
			pop ebx
		}
	}

	void __stdcall kill_effect(void* player, int id) {
		uintptr_t fn = 0x0081FAF0;   // uPlayer::killEffectId (this@eax, id; retn 4)
		if (!player) return;
		_asm {
			pushad
			mov eax, player
			push id
			call fn
			popad
		}
	}

	void __stdcall finish_effect(void* player, int id) {
		uintptr_t fn = 0x0081FB70;   // uPlayer::finishEffectId (this@eax, id, bool; callee pops)
		if (!player) return;
		_asm {
			push ebx
			mov ebx, esp
			pushad
			mov eax, player
			push 1
			push id
			call fn
			popad
			mov esp, ebx
			pop ebx
		}
	}

	uint32_t __stdcall ground_material(void* player) {
		uintptr_t fn = 0x00820060;   // uPlayer::getEfctMtrlFlg (this@eax)
		uint32_t r = 0;
		if (!player) return 0;
		_asm {
			pushad
			mov eax, player
			call fn
			mov r, eax
			popad
		}
		return r;
	}

	void* __stdcall held_weapon(void* player) {
		uintptr_t fn = 0x007B1F90;   // uPlayer::cWeaponCtrl::getHeldWeapon (&mWeaponCtrl@esi)
		void* r = nullptr;
		if (!player) return nullptr;
		_asm {
			pushad
			mov esi, player
			add esi, 0x1D88
			call fn
			mov r, eax
			popad
		}
		return r;
	}

	void __stdcall play_player_se(void* player, int id) {
		uintptr_t fn_requestSe    = 0x00A47AA0;   // sDevil4Sound::requestSe(inst, rsrc, id, owner, pos*, 1, 1, cb) stdcall
		uintptr_t fn_sePositionCb = 0x00486640;   // sSoundN::soundPositionUpdateCb
		const uintptr_t inst = *(uintptr_t*)0x00E559D8;   // sSoundN::mpInstance
		if (!inst || !player) return;
		const uintptr_t rsrc = *(uintptr_t*)((uintptr_t)player + 0x1D60);
		if (!rsrc) return;
		const float* pos = (const float*)((uintptr_t)player + 0x30);   // uCoord::mPos (16 bytes, requestSe copies two qwords)
		((void(__stdcall*)(uintptr_t, uintptr_t, int, void*, const float*, int, int, uintptr_t))fn_requestSe)(
			inst, rsrc, id, player, pos, 1, 1, fn_sePositionCb);
	}

	void __stdcall indexed_anim_call(uint32_t id, uPlayer* actor, uint32_t mode, float speed,
									float startFrame, float interpolationFrame) {
        uintptr_t anim_call = 0x00821450;
		uintptr_t curr_esp;
		_asm {
				mov [curr_esp],esp
				sub esp,0x0C

				fld speed
				fstp dword ptr [esp+0x8]
				fld startFrame
				fstp dword ptr [esp+0x4]
				fld interpolationFrame
				fstp dword ptr [esp]

				mov al,1
				mov ecx,[actor]
				push mode //00-normal,0x15-looped
				push id

				call anim_call
				//mov esp,[curr_esp]
		}
	}

	void __stdcall neutral_air_transition(uPlayer* uActor) {
            float float_val1 = 0.98f;
            float float_val2 = 1000000.0f;
            float float_val3 = 20.0f;

            uintptr_t neutral_air_recovery_call = 0x7AA410;
			uintptr_t curr_esp = 0;
			_asm {
					mov eax,uActor
					mov [curr_esp],esp

					fld dword ptr [float_val1]
					movss xmm1,[float_val2]

					sub esp,0x10
					mov [esp+0xC],1
					fld dword ptr [eax+0xEC4]
					fstp dword ptr [esp+0x20]
					fstp dword ptr [esp+0x8]
					mov edi,8
					fld dword ptr [esp+0x20]
					fstp dword ptr[esp+0x4]
					fld dword ptr [float_val3]
					fstp dword ptr [esp]
					call neutral_air_recovery_call
					mov esp,[curr_esp]
			}
    }

	MtObject* __stdcall get_stuff_from_files(MtDTI* dti, char* path, uint32_t mode) {
		constexpr uintptr_t file_read_call = 0x008DF530;
		_asm {
				push mode
				push path
				push dti
				mov eax,sDevil4Resource
				mov eax,[eax]
				call file_read_call
		}
	}

	// cResource::release(sResource* this@eax, cResource* rsrc@edi) 0x8DDA00, __usercall: refcount--, destroyed at 0.
	// (Until 2026-10-18 this pushed rsrc on the stack and passed the address of mpInstance in eax, which made
	// the game decrement a garbage refcount and enter a bogus critical section: a hang on the spot.)
	void __stdcall release_resource(cResource* rsrc) {
		uintptr_t release_call = 0x8DDA00;
		_asm {
				pushad
				mov edi, rsrc
				mov eax, sDevil4Resource
				mov eax, [eax]
				call release_call
				popad
		}
	}

	// uModel::setMotionList(this@ecx, cResource* lmt, int bank) 0xA26340, __thiscall: releases the slot's old list,
	// addRefs the new one, stores it and flags every blend slot for re-bind. Called through a __fastcall cast
	// (ecx = this, edx = dummy) so no asm is needed.
	static void set_motion_list(void* model, void* lmt, int bank) {
		((void(__fastcall*)(void*, int, void*, int))0x00A26340)(model, 0, lmt, bank);
	}

	bool __stdcall load_motion_bank(void* model, const char* lmt_path, int bank) {
		if (!model || bank < 0 || bank > 15) return false;
		MtDTI* rMotionListDTI = (MtDTI*)0x00EAE348;
		MtObject* lmt = get_stuff_from_files(rMotionListDTI, (char*)lmt_path);
		if (!lmt) {
			spdlog::error("load_motion_bank: '{}' did not load", lmt_path);
			return false;
		}
		set_motion_list(model, lmt, bank);        // the slot now holds its own reference
		release_resource((cResource*)lmt);        // drop the one sResource::create gave us
		return true;
	}

	void __stdcall unload_motion_bank(void* model, int bank) {
		if (model && bank >= 0 && bank <= 15) set_motion_list(model, nullptr, bank);
	}

	void __stdcall borrow_ctn(void* player, uint32_t bank, uint32_t idx) {
		for (int half = 0; half < 2; ++half) {
			const uintptr_t p = (uintptr_t)player + 0x2A3C + 12 * half;   // mMotionSetCharTblParam[half]
			*(bool*)(p + 0) = true;
			*(uint32_t*)(p + 4) = bank;
			*(uint32_t*)(p + 8) = idx;
		}
	}

	void __stdcall install_ctn_script(void* player, const void* script) {
		for (int half = 0; half < 2; ++half) {
			const uintptr_t mgr = (uintptr_t)player + 0x1DD4 + 0x24 * half;   // cCharTblMgr[half]
			*(const uint8_t**)(mgr + 0x0C) = (const uint8_t*)script;   // mpDataTop
			*(const uint8_t**)(mgr + 0x10) = (const uint8_t*)script;   // mpDataCrnt
			*(bool*)(mgr + 0x14) = false;                               // mIsDone
		}
	}

	void __stdcall install_cancel_script(void* player, CancelScript& script, float frame) {
		script.wait_frame = frame;
		install_ctn_script(player, &script);
	}

	uintptr_t __stdcall get_motion_bank(void* model, int bank) {
		if (!model || bank < 0 || bank > 15) return 0;
		return *(uintptr_t*)((uintptr_t)model + 0xA38 + 4 * bank);
	}

	MtObject* __stdcall uEm010ShlCtrl_constructor_sub(void* projectile) {
		uintptr_t constructor_call = 0x4A7E80;
		uintptr_t uEm010ShlCtrl_methods_ptr = 0xBCE648;
		_asm {
				mov esi,projectile
				call constructor_call
				mov [esi+0x1370],0
				push uEm010ShlCtrl_methods_ptr
				pop [esi]
				mov [esi+0xEA4],5
		}
		return (MtObject*)projectile;
	}

	_declspec(naked) void* __cdecl MemberFuncToPtr(...) {
		__asm {
		mov eax, [esp+4]
		ret
		}
	}

	void __stdcall bring_assert(void* rFile) {
		uintptr_t assert_call = 0x008DDA00;
		uintptr_t sDevil4Resource_ptr = 0x00E552D0;
		_asm {
		mov eax,[sDevil4Resource_ptr]
		mov eax,[eax]
		mov edi,[rFile]
		call assert_call
		}
	}
	//uintptr_t __stdcall player_uEm010ShlCtrl_spawner(uPlayer* actor, uint32_t mode) { //0-pillars, 1-wave
	//	float WaveRange = 220.0f;
	//	float WaveInterval = 3.0f;
	//	float WaveAngle = 0.04;
	//	float WaveAngleAlt = 0.0f;
	//	float WaveSpeed = 25.0f;
	//	float WaveSpread = 55.0f;
	//	float PillarRange = 240.0f;
	//	float PillarAngle = 0.7;
	//	float PillarSpread = 60.0f;
	//	void* mem = mt_allocate_heap(sizeof(uEm003Shl), 0x10);
	//}

}

namespace uactor_sdk {

	void __stdcall load_atk_col(void* rAtck, void* rCol, void* ColMgr, void* Obj) {
		uintptr_t load_call = 0x0050BDD0;
		_asm {
			pushad
			push [rAtck]
			push [rCol]
			mov esi,[ColMgr]
			mov eax,[Obj]
			call load_call
			popad
		}
	}

	void __stdcall get_model(void* obj, void* model) {
		_asm {
			pushad
			mov ecx,[obj]
			mov eax,[ecx]
			mov edx,[eax+0x40]
			mov eax,[model]
			push eax
			call edx
			popad
		}
	}

	void __stdcall updateLmat(void* obj) {
		_asm {
			pushad
			mov ecx,[obj]
			mov eax,[ecx]
			mov edx,[eax+0x34]
			call edx
			popad
		}
	}

	void __stdcall updateWmat(void* obj) {
		_asm {
			pushad
			mov ecx,[obj]
			mov eax,[ecx]
			mov edx,[eax+0x38]
			call edx
			popad
		}
	}

	// this is safe to be called if the obj does not exist
	void __stdcall despawn(void* obj) {
		_asm {
			mov esi,[obj]
			mov eax,[esi+4]
			and eax,-5
			or eax,3
			mov [esi+4],eax
		}
	}

	void __stdcall ushell_des(void* obj) {
		uintptr_t ushellDTI         = 0x00BEE7C0;
		uintptr_t ushell_destructor = 0x004A7280;
		_asm {
			pushad
			mov ecx,[obj]
			mov esi,ecx
			mov edx, ushellDTI
			mov [esi],edx
			call ushell_destructor
			popad
		}
	}

	void __stdcall uActorCons(void* obj) {
		uintptr_t uActor_constructor = 0x004A6E80;
		_asm {
			mov esi,[obj]
			call uActor_constructor
		}
	}

	void __stdcall uCollisionMgrCons(void* obj) {
		uintptr_t uCollisionMgr_constructor = 0x0050B080;
		_asm {
				mov ecx,[obj]
				call uCollisionMgr_constructor
		}
	}

	void __stdcall collide(void* CollMgr) {
		_asm {
				pushad
				mov ecx,[CollMgr]
				mov eax,[ecx]
				mov edx,[eax+0x18]
				call edx
				popad
		}
	}

	void __stdcall hitbox_call(void* CollMgr, int id) {
		uintptr_t hitbox_call_func = 0x0050CA60;
		float timer                = 5.0f;
		_asm {
				pushad
				push 00
				push [CollMgr]
				or edx,-1
				mov eax,[id]
				call hitbox_call_func
				test eax,eax
				je return_addr
				movss xmm0,[timer]
				movss [eax+0x14C],xmm0
				xorps xmm0,xmm0
				movss [eax+0x148],xmm0
				mov esi,[CollMgr]
				mov esi,[esi+0xD8]
				lea edx,[esi+0xA0]
				mov [eax+0x140],edx
				mov byte ptr [eax+0x64],1
			return_addr:
				popad
		}
	}

	void __stdcall render_call(void* render_obj, void* trans) {
		uintptr_t render_func = 0x00522DC0;
		_asm {
				pushad
				push [trans]
				mov ecx, [render_obj]
				call render_func
				popad
		}
	}
	
	void __stdcall uDevil4ModelCons(void* obj) {
        uintptr_t Devil4Model_constructor = 0x005226F0;
        _asm {
				mov ecx,[obj]
				call Devil4Model_constructor
        }
	}
    void __stdcall uDevil4ModelDest(void* obj) {
        uintptr_t Devil4Model_destructor = 0x05229B0;
        _asm {
			mov ecx,[obj]
			call Devil4Model_destructor
        }
    }

	void __stdcall destructor_call(void* obj) {
		_asm {
			pushad
			mov ecx,[obj]
			mov esi,[ecx]
			mov edx,[esi]
			push 01
			call edx
			popad
		}
	}
	void* __stdcall get_joint_from_index(void* obj, int index) {
		_asm {
			pushad
			mov ecx,[obj]
			mov eax,[index]
			mov esi,[ecx+0x2E4]
			mov eax,[eax+eax*8]
			shl eax,4
			mov eax,[esi+eax]
			ret
		}
	}
	void* __stdcall get_center_pos(void* obj, Vector3f* pos) {
		void** vtable = *(void***)obj;
		typedef void(__thiscall* VirtualFn)(void*, Vector3f*);
		((VirtualFn)vtable[33])(obj, pos);
		return pos;
	}
}
