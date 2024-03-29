
#include "Devil4.hpp"

static MtHeapAllocator** mt_heap_alloc_static_ptr = (MtHeapAllocator**)0x00E1434C;

static uintptr_t fptr_update_actor_list{ 0x008DC540 }; // Spawns shit
static uintptr_t some_struct{ 0x00E552CC };
static uintptr_t sKeyboard{ 0x00E559C0 };


class sAreaStaticPtr
{
public:
    class sArea *sAreaPtr; //0x0000
    char pad_0004[60]; //0x0004
}; //Size: 0x0040
static_assert(sizeof(sAreaStaticPtr) == 0x40);

class sArea
{
public:
    char pad_0000[14384]; //0x0000
    class aGame *aGamePtr; //0x3830
    char pad_3834[16]; //0x3834
}; //Size: 0x3844
static_assert(sizeof(sArea) == 0x3844);

class aGame
{
public:
    char pad_0000[464]; //0x0000
    int8_t unk; //0x01D0
    int8_t m_paused; //0x01D1
    char pad_01D2[2]; //0x01D2
}; //Size: 0x01D4
static_assert(sizeof(aGame) == 0x1D4);

namespace devil4_sdk {

	void* mt_allocate_heap(size_t size, int a2) {
		static MtHeapAllocator* mt_heap_alloc = *mt_heap_alloc_static_ptr;

		return (void*)mt_heap_alloc->allocate(size, a2);
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

	void u_em003_shl_new() {
		void* mem = mt_allocate_heap(sizeof(UEm003Shl), 0x10);
		if (!mem) {
			return;
		}
		
		UEm003Shl* shell = (UEm003Shl*)u_em003_shl_consturctor_sub(mem);
		if (!shell) {
			return;
		}

		spawn_or_something((void*)0x00E552CC, shell, 0x12);

		shell->m_some_float_vec = glm::vec3(0.0f, 0.0f, 0.0f);
		
		auto lp = devil4_sdk::get_local_player();
		shell->m_pos_maybe = lp->m_pos;

		shell->m_required_idk_wtf_is_this_needs_to_be2 = 2;
		shell->m_required_idk_wtf_is_this_needs_to_be6 = 0;
		shell->m_required_some_byte_idk = 0;
	}

	uPlayer* get_local_player() {
		constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
		static SMediator* s_mediator_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
		return s_mediator_ptr->player_ptr;
	}

	uCameraCtrl* get_local_camera() {
        constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
        static SMediator* s_mediator_ptr        = (SMediator*)*(uintptr_t*)static_mediator_ptr;
        return s_mediator_ptr->camera1;
    }

    cCameraPlayer* get_player_camera() {
        constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
        static SMediator* s_mediator_ptr        = (SMediator*)*(uintptr_t*)static_mediator_ptr;
        return s_mediator_ptr->camera1->cCameraPlayer1;
    }

	SStylishCount* get_stylish_count() {
		constexpr uintptr_t s_stylish_count_ptr = 0x00E558CC;
		static SStylishCount* sc = (SStylishCount*)*(uintptr_t*)s_stylish_count_ptr;
		return sc;
	}

	float get_current_style_rank() {
		SStylishCount* sc = get_stylish_count();
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

        static sAreaStaticPtr* ptr = (sAreaStaticPtr*)0x00E552C8;
        __try {
            return ptr->sAreaPtr->aGamePtr->m_paused;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            return true;
        }
        
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

	std::pair<uint16_t, const char*> getButtonInfo(uint16_t buttonNum) {
        for (const auto& pair : buttonPairs) {
			if (pair.first == buttonNum) {
            return pair;
			}
        }
            return buttonPairs[0];
    }

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
                        }	}
}