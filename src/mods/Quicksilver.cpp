#include "Quicksilver.hpp"
#include "glm/gtx/compatibility.hpp"
#include "../sdk/Devil4.hpp"

bool Quicksilver::mod_enabled_nero = false;
bool Quicksilver::mod_enabled_dante = false;
utility::Timer* Quicksilver::m_timer{};

// Found out that the game does not load anything
// not related to current stage, after writing
// all this nonsence
#if 0
void spawnOrSomething(MtObject* toSpawn) {
	constexpr uintptr_t uSomeGlob = 0x00E552CC;
	constexpr uintptr_t spawnShit = 0x008DC540;
	
	__asm {
		mov eax, uSomeGlob
		mov eax, [eax]
		push 0xA
		mov esi, toSpawn
		call spawnShit
	}
}

rEffectList* LoadEffectAsmCall() {

	uintptr_t sDevil4ResourcePtr = 0x00E552D0;
	uintptr_t MtDtiPtr= 0x00EADE88;
	const char* path = "effect\\efl\\scr\\es402_81v0";
	uintptr_t LoadEffectResourceFptr = 0x008DF530;

	rEffectList* asmRet = nullptr;

	__asm {
		mov eax, [sDevil4ResourcePtr]
		mov eax, [eax]
		push 01
		mov ebx, [path]
		push ebx
		mov ebx, [MtDtiPtr]
		push ebx
		call LoadEffectResourceFptr
		mov [asmRet], eax
	}
	return asmRet;
}

uEfctCam* getEffctCam(MtObject* mto) {

	uEfctCam* returnValue = nullptr;
	uintptr_t funcPtr = 0x00539270;

	__asm {
		mov esi, [mto]
		call funcPtr
		mov [returnValue], eax
	}

	return returnValue;
}


struct someArg {
	glm::vec3 one { 0.0f, 0.0f, 0.0f };
	glm::vec4 two { 1.0f, 0.0f, 0.0f, 0.0f };
};


void someWeirdAssFunction(someArg arg, uEfctCam* uCam, MtObject* alloced) {
	
	uintptr_t funcPtr = 0x0096A0A0;
	__asm {
		push 0
		lea ecx, arg
		push arg
		push -1
		push -1
		push alloced
		push uCam
		mov eax, 4
		or ecx, -1
		call funcPtr
	}
}

void screenBlinkEffect() {
	if (LoadEffectAsmCall()) {

		uintptr_t staticMtHeapAllocPtr = 0x00E1434C;
		MtHeapAllocator* mth = *(MtHeapAllocator**)staticMtHeapAllocPtr;

		MtObject* allocated = mth->allocate(0x210, 0x10);
		if (allocated) {
			uEfctCam* cam = getEffctCam(allocated);
			if (cam) {
				cam->someField = 1;
				someArg arg{};
				someWeirdAssFunction(arg, cam, allocated);
				spawnOrSomething((MtObject*)cam);
			}
		}
	}
}
#endif

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

struct PostProcessingEffects {
    UTvNoiseFilter* tv;
    UColorCorrectFilter* cc;
};

PostProcessingEffects pps;
UStageSetTimeSlow* g_ss;

void tv_noise_constructor_params(UTvNoiseFilter* tv) {
    tv->m_priority ^= 2048;
    tv->m_noise_power            = 0.06f;
    tv->m_shock_noise_freq       = 0.02f;
    tv->m_v_sync_noise_amplitude = 0.005f;
}

void u_color_correct_constructor_params(UColorCorrectFilter* cc) {
    cc->m_priority ^= 2048;

    /*cc->correctors[0].mType = CC_TYPE::TYPE_NEGA2;
    cc->correctors[0].mEnable = true;*/

    cc->correctors[0].m_type   = CcType::TYPE_CHROMA;
    cc->correctors[0].m_factor = glm::vec4{1.0f, 0.1f, 0.1f, 0.1f};
    cc->correctors[0].m_enable = true;
}

void u_stage_set_time_slow_constructor_params(UStageSetTimeSlow* ss) {
    g_ss           = ss;
    ss->m_duration = 1100.0f; // hardcoded for now idk what they used
}
bool start_interpolating = false;

void Quicksilver::qs_operator_new() {

    constexpr uintptr_t u_stage_slow_constructor           = 0x00887440;
    constexpr uintptr_t u_some_glob                        = 0x00E552CC;
    constexpr uintptr_t spawn_shit                         = 0x008DC540;
    constexpr uintptr_t u_color_correct_filter_constructor = 0x0093D9A0;
    constexpr uintptr_t u_tv_noise_filter_constructor      = 0x00922290;
    float tv_noise_arg1                                    = 0.06f;
    float tv_noise_arg2                                    = 0.02f;
    float tv_n_oise_arg3                                   = 0.005f;
    start_interpolating                                    = true;

    __asm {
		pusha
		pushf
            // call screenBlinkEffect // not working on 4 lmao
            // noise
		call u_tv_noise_filter_constructor
                /*xor [eax+0x4], 0x00000800
                mov edx, [tvNoiseArg1]
                mov [eax+0x34], edx
                mov edx, [tvNoiseArg2]
                mov [eax+0x68], edx
                mov edx, [tvNOiseArg3]
                mov [eax+0x6C], edx
                mov [eax+0x44], 0x00000000*/
		push eax
		call tv_noise_constructor_params
		pop eax
		mov esi,eax
		test esi, esi
		jz bounce

		mov dword ptr [pps.tv], esi
		mov eax, u_some_glob
		mov eax, [eax]
		push 0x1D
		call spawn_shit

            // cc
		call u_color_correct_filter_constructor
		pusha
		push eax
		call u_color_correct_constructor_params
		pop eax
		popa
                /*mov ecx, 00000800h
                xor DWORD PTR [eax+0x4], ecx
                mov ecx, 16
                mov DWORD PTR [eax+0x58], ecx
                mov [eax+0x74], 0x00000004
                mov [eax+0x80], 0x00000000
                mov [eax+0x84], 0x00000000
                mov [eax+0x88], 0x00000000*/

		mov esi, eax
		test esi, esi
		jz bounce
		mov dword ptr [pps.cc], esi
		mov eax, u_some_glob
		mov eax, [eax]
		push 0x1D
		call spawn_shit

            // slowdown

		call u_stage_slow_constructor
		pusha
		push eax
		call u_stage_set_time_slow_constructor_params
		pop eax
		popa
		mov esi, eax
		test esi, esi
		jz bounce
		mov eax, u_some_glob
		mov eax, [eax]
		push 01
		call spawn_shit
	bounce:
		popf
		popa
    }
}

static void on_timer_callback() {
    g_ss->m_time_left = 0.0f;
    pps.tv->bitfield ^= 2048;
    pps.cc->bitfield ^= 2048;
}

std::optional<std::string> Quicksilver::on_initialize() {
    /*m_command = std::hash<std::string>{}("\\Quicksilver");
    m_shorthand = std::hash<std::string>{}("\\qs");*/
    MutatorRegistry::define("Quicksilver")
        .alias("qs")
        .on_init([this]() {
            SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
            uPlayer* u_local_plr = s_med_ptr->player_ptr;
            if (u_local_plr) {
                Quicksilver::qs_operator_new();
            }
        })
        .set_timer(15.0f, []() { on_timer_callback(); });

    // timer duration in float and callback function once it finishes
    m_timer = new utility::Timer(15.0f, on_timer_callback);
	utility::create_keyboard_hotkey(Quicksilver::m_hotkeys, {VK_OEM_PLUS}, "QuickSilver", "quicksilver_key");

    return Mod::on_initialize();
}

void Quicksilver::on_frame(fmilliseconds& dt) {
    if (m_timer) {
        m_timer->tick(dt);
    }
}

void Quicksilver::on_gui_frame(int display) {
	if (display == 1) {
		ImGui::Checkbox(_("Quicksilver"), &mod_enabled_nero);
		ImGui::SameLine();
		help_marker(_("Enables the hotkey for Quicksilver. By default this is = / +"));
	}
	else if (display == 2) {
		ImGui::Checkbox(_("Quicksilver"), &mod_enabled_dante);
		ImGui::SameLine();
		help_marker(_("Enables the hotkey for Quicksilver. By default this is = / +"));
	}
}
#if 0
void Quicksilver::on_twitch_command(std::size_t hash) {
    // if (!m_timer->m_active) { return; }
    if (hash == m_command || hash == m_shorthand) {
        if (m_timer) {
            if (m_timer->m_active == false) {
                SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
                uPlayer* u_local_plr = s_med_ptr->player_ptr;
                if (u_local_plr) {
                    qs_operator_new();
                    m_timer->start();
                }
            }
        }
    }
}
#endif
void Quicksilver::on_update_input(utility::Input& input) {
	if (uPlayer* player = devil4_sdk::get_local_player()) {
		if (Quicksilver::m_hotkeys[0]->check(input)) {
			if (mod_enabled_nero) {
				if (player->controllerID == 1) {
					if (m_timer) {
						if (m_timer->m_active == false) {
							qs_operator_new();
							m_timer->start();
						}
					}
				}
			}
			if (mod_enabled_dante) {
				if (player->controllerID == 0) {
					if (m_timer) {
						if (m_timer->m_active == false) {
							qs_operator_new();
							m_timer->start();
						}
					}
				}
			}
		}
	}
}

void Quicksilver::on_config_load(const utility::Config& cfg) {
    mod_enabled_nero = cfg.get<bool>("quicksilver_enabled_nero").value_or(false);
	mod_enabled_dante = cfg.get<bool>("quicksilver_enabled_dante").value_or(false);
}

void Quicksilver::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("quicksilver_enabled_nero", mod_enabled_nero);
	cfg.set<bool>("quicksilver_enabled_dante", mod_enabled_dante);
}
