#include "GermanWord.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sUnit.hpp"
#include "CharSwitcher.hpp" // for external_spawn_requested, stops chars being registered to sMed

static constexpr uintptr_t some_struct            = 0x00E552CC;
static constexpr uintptr_t fptr_update_actor_list = 0x008DC540;

bool GermanWord::mod_enabled = false;
bool GermanWord::survival_doppel_enabled = false;
bool GermanWord::spawn_queued = false;

static constexpr uintptr_t danteSpawnAddr = 0x7B2130;
static constexpr uintptr_t neroSpawnAddr  = 0x7E1A70;
static uPlayer* doppelAddr          = nullptr;

static bool spawn_ai_dante_toggle = false;
static bool spawn_ai_nero_toggle = false;

// let players push players
void GermanWord::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x10C6B9, patch1, "\x8B\x91\xE0\x02\x00\x00", 6); // mVsAttrPlPush to mVsAttrEmPush - mov edx,[ecx+000002E0] (0x44444444)
    }
    else {
        patch1.reset(); // mVsAttrPlPush - mov edx,[ecx+000002B8] (0x44444400)
    }
}

// comboing these with other edits let you change doppel to be the ai chara but I do not remember the sick pause combo to do it (you literally have to pause)
void GermanWord::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x838078, patch2, "\xFB\xFB\x30\x06", 4);
    } else {
        install_patch_offset(0x838078, patch2, "\xF8\xF0\x1D\x00", 4);
        // patch2.reset();
    }
}

void GermanWord::toggle3(bool enable) {
    if (enable) {
        install_patch_offset(0x838078, patch3, "\xFB\xFB\x30\x05", 4);
    } else {
        install_patch_offset(0x838078, patch3, "\xF8\xF0\x1D\x00", 4);
        // patch3.reset();
    }
}

static void __stdcall update_spawn_pos(uPlayer* player1, Vector3f* doppelPos, float* doppelRot) {
    float yaw = player1->rotation2;
    doppelPos->x = player1->mPos.x + cosf(yaw) * -100.0f;
    doppelPos->y = player1->mPos.y;
    doppelPos->z = player1->mPos.z - sinf(yaw) * -100.0f;
    *doppelRot   = yaw;
}

static void spawn_dante() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player /* || player->controllerID == 1*/) {
        return;
    }
    __asm {
		pushad
		pushfd
        mov byte ptr [CharSwitcher::external_spawn_requested], 1
        call dword ptr [danteSpawnAddr]
        mov [doppelAddr], eax
        mov esi, eax

        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        lea ebx, [eax+0x70]
        push ebx
        lea ebx, [eax+0x60]
        push ebx
        mov eax, [player]
        push eax
        call update_spawn_pos

        mov eax, [some_struct]
        mov eax, [eax]
        push 13 // players
        call fptr_update_actor_list

		popfd
		popad
    }
}

static void spawn_nero() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player /* || player->controllerID == 0*/) {
        return;
    }
    __asm {
		pushad
		pushfd
        mov byte ptr [CharSwitcher::external_spawn_requested], 1
        call dword ptr [neroSpawnAddr]
        mov [doppelAddr], eax
        mov esi, eax

        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        lea ebx, [eax+0x70] // rot
        push ebx
        lea ebx, [eax+0x60] // pos
        push ebx
        mov eax, [player]
        push eax
        call update_spawn_pos

        mov eax, [some_struct]
        mov eax, [eax]
        push 13 // players
        call fptr_update_actor_list
		popfd
		popad
    }
}

class ParentOf {
public:
    char pad_00[0x04];
    ParentOf* prev; // 0x04
    ParentOf* next; // 0x08
    char pad_0c[0x17a4];
    uPlayer* parent; // 0x17b0
    char pad_0x17b4[0x29];
    bool die; // 0x17dd
    char pad_0x17de[0x6f];
    bool alive2; // 0x184c
};
static_assert(sizeof(ParentOf) == 0x1850);

typedef void(__thiscall* VirtualFn)(void*);
static void call_vfunc12(void* obj) {
    void** vtable = *(void***)obj;
    ((VirtualFn)vtable[12])(obj);
}

static void despawn_owned_projectiles(void* owner) {
    if (!owner)
        return;
    auto* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit)
        return;

    auto* mlEntry = (ParentOf*)sUnit->mMoveLine[18].mTop; // projectiles

    while (mlEntry) {
        auto* next = mlEntry->next;

        if (mlEntry->parent == owner) {
            //uactor_sdk::despawn(mlEntry);
            call_vfunc12(mlEntry);
        }
        mlEntry = next;
    }
}

class uPlWpRightHand {
public:
    char pad_00[0x2720];
    uEnemy_Old* heldEnemy; // 0x2720
};
static_assert(sizeof(uPlWpRightHand) == 0x2724);

class NeroStand {
public:
    char pad_00[0x1374];
    uPlWpRightHand* rightHand; // 0x1374
    char pad_1378[0xf48];
    int int22c0; // 0x22c0
    char pad_22c4[0x186];
    int int244A; // 0x244a
};
static_assert(sizeof(NeroStand) == 0x2450);

static int pending_nero_despawn = 0;
void GermanWord::DoppelSpawnLogic(bool enabled) {
    toggle1(enabled);
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    spawn_queued = false;
    int id = player->controllerID;
    if (enabled) {
        if (id == 0) {
            if (!doppelAddr || !devil4_sdk::check_exists_in_moveline(doppelAddr, 13)) {
                spawn_dante();
            }
        } else {
            if (!doppelAddr || !devil4_sdk::check_exists_in_moveline(doppelAddr, 13)) {
                spawn_nero();
            }
        }
    } else {
        if (id == 0) {
            if (doppelAddr && devil4_sdk::check_exists_in_moveline(doppelAddr, 13)) {
                uPlayer* doppel = (uPlayer*)doppelAddr;
                void* addr = doppel->lucifer;
                call_vfunc12(addr);

                addr = doppel->pandora;
                call_vfunc12(addr);

                despawn_owned_projectiles(doppelAddr);
                call_vfunc12(doppelAddr);
                doppelAddr = nullptr;
            } else {
                doppelAddr = nullptr;
            }
        } else {
            if (doppelAddr && devil4_sdk::check_exists_in_moveline(doppelAddr, 13)) {
                if (pending_nero_despawn == 0) {
                    uPlayer* doppel      = (uPlayer*)doppelAddr;
                    NeroStand* stand     = (NeroStand*)doppel->stand;
                    stand->int22c0       = 50;
                    pending_nero_despawn = 1;
                }
            } else {
                doppelAddr = nullptr;
            }
        }
    }
}

void GermanWord::on_frame(fmilliseconds& dt) {
    if (mod_enabled || survival_doppel_enabled) {
        if (spawn_queued) {
            DoppelSpawnLogic(true);
        }
    } else {
        DoppelSpawnLogic(false);
    }

    if (pending_nero_despawn > 0) {
        pending_nero_despawn++;
        if (pending_nero_despawn > 3) {
            if (doppelAddr && devil4_sdk::check_exists_in_moveline(doppelAddr, 13)) {
                uPlayer* doppel  = (uPlayer*)doppelAddr;
                NeroStand* stand = (NeroStand*)doppel->stand;
                call_vfunc12(stand);
                despawn_owned_projectiles(doppel);
                call_vfunc12(doppel);
            }
            doppelAddr = nullptr;
            pending_nero_despawn = 0;
        }
    }
}

void GermanWord::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Doppelganger##GermanWord"), &mod_enabled)) {
            spawn_queued = true;
        }
        #ifndef NDEBUG
        if (ImGui::Checkbox(_("Become AI Dante##GermanWord"), &spawn_ai_dante_toggle)) {
            toggle2(spawn_ai_dante_toggle);
        }
        ImGui::SameLine();
        help_marker("Tick then untick");
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Become AI Nero##GermanWord"), &spawn_ai_nero_toggle)) {
            toggle3(spawn_ai_nero_toggle);
        }
        ImGui::SameLine();
        help_marker("Tick then untick");
        #endif
    }
}

void GermanWord::on_stage_start() {
    if (mod_enabled) {
        doppelAddr  = nullptr;
        spawn_queued = true;
        survival_doppel_enabled = false;
    }
}

// void GermanWord::on_game_pause(bool toggle) {}
// bool GermanWord::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> GermanWord::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init GermanWord mod 1\n");
        return "Failed to init GermanWord mod 1";
	}*/

    return Mod::on_initialize();
}

void GermanWord::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("GermanWord").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void GermanWord::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("GermanWord", mod_enabled);
}
