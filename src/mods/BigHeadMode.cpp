#include "BigHeadMode.hpp"
#include "../sdk/Players.hpp"
#include "glm/gtx/compatibility.hpp"
#include "../sdk/Devil4.hpp"
#include "PhotoMode.hpp"

bool BigHeadMode::big_head_mode_nero = false;
bool BigHeadMode::swole_mode_nero = false;

bool BigHeadMode::big_head_mode_dante = false;
bool BigHeadMode::swole_mode_dante = false;

static uintptr_t joint_size_detour1_continue = NULL;
static uintptr_t joint_size_detour2_continue = NULL;

glm::vec3 size{ 0.5f, 0.5f, 0.5f };
glm::vec3 size_torso{ 0.9f, 0.8f, 0.8f };

static void scale_head_joint(UModelJoint* joint, uPlayer* player) {
    glm::vec3& scale = *(glm::vec3*)&joint->mScale;
    
    if (player->controllerID == 1) {
        if (BigHeadMode::swole_mode_nero) {
            scale = size * glm::clamp(devil4_sdk::get_current_style_rank(), 1.5f, 3.0f);
        }
        else if (BigHeadMode::big_head_mode_nero) {
            scale = size * devil4_sdk::get_current_style_rank();
        }
    }
    if (player->controllerID == 0) {
        if (BigHeadMode::swole_mode_dante) {
            scale = size * glm::clamp(devil4_sdk::get_current_style_rank(), 1.5f, 3.0f);
        }
        else if (BigHeadMode::big_head_mode_dante) {
            scale = size * devil4_sdk::get_current_style_rank();
        }
    }
}

// The player this joint is the head (big head) or torso (swole) of, under his own character's toggles; null for any
// other joint. Every player is checked, so it doesn't matter which context the joint update runs in.
static uPlayer* joint_owner(UModelJoint* joint) {
    for (int i = 0; i < player_info::count(); i++) {
        uPlayer* u_plr = (uPlayer*)player_info::player(i);
        if (!u_plr || !u_plr->joint_array) continue;
        const bool dante = u_plr->controllerID == 0, nero = u_plr->controllerID == 1;
        const bool swole = (dante && BigHeadMode::swole_mode_dante) || (nero && BigHeadMode::swole_mode_nero);
        const bool big   = (dante && BigHeadMode::big_head_mode_dante) || (nero && BigHeadMode::big_head_mode_nero);
        if (swole) {
            if (joint == &u_plr->joint_array->joint[2]) return u_plr; // seems to be torso for both chars
        } else if (big) {
            if (joint == &u_plr->joint_array->joint[4]) return u_plr; // seems to be heads for both chars
        }
    }
    return nullptr;
}

// 1 = the joint was a player's head / torso and is scaled (the original scale write is skipped)
static int __cdecl scale_if_head(UModelJoint* joint) {
    uPlayer* owner = joint_owner(joint);
    if (!owner) return 0;
    scale_head_joint(joint, owner);
    return 1;
}

naked void joint_size_detour1() {
	__asm {
		pushfd
		cmp byte ptr [PhotoMode::mod_enabled], 1
		je bailpopfd
		pushad
		push esi // joint
		call scale_if_head
		add esp, 4
		test eax, eax
		popad
	    je originalCode1
	bailpopfd:
		popfd
		jmp bail
	originalCode1:
		popfd
		movss [esi+0x30], xmm0
		movss xmm0, [esp+0x54]
		movss [esi+0x34], xmm0
		movss xmm0, [esp+0x58]
		movss [esi+0x38], xmm0
	bail:
		jmp dword ptr [joint_size_detour1_continue]
	}
}

naked void joint_size_detour2() {
	__asm {
		pushfd
		cmp byte ptr [PhotoMode::mod_enabled], 1
		je bailpopfd
		pushad
		push esi // joint
		call scale_if_head
		add esp, 4
		test eax, eax
		popad
		je originalCode2
	bailpopfd:
		popfd
		jmp bail

	originalCode2:
		popfd
		movss [esi+0x30], xmm0
		movss xmm0, [esp+1C4h]
		movss [esi+0x34],xmm0
		movss xmm0,[esp+1C8h]
		movss [esi+0x38],xmm0

	bail:
		jmp dword ptr [joint_size_detour2_continue]
	}
}


std::optional<std::string> BigHeadMode::on_initialize(){
#if 1
    MutatorRegistry::define("BigHeadMode")
        .on_init( []() {
        big_head_mode_nero = !big_head_mode_nero;
    }).set_timer(300.0, []() {
        big_head_mode_nero = false;
    });

    MutatorRegistry::define("SwoleMode")
        .on_init( []() {
        big_head_mode_nero = !big_head_mode_nero;
        swole_mode_nero = true;
    }).set_timer(300.0, []() {
        big_head_mode_nero = false;
        swole_mode_nero = false;
    });

	// DevilMayCry4_DX9.exe+5E9774 
	if (!install_hook_offset(0x5E9774 , hook1, &joint_size_detour1, &joint_size_detour1_continue, 0x1B)) {
		spdlog::error("Failed to init BigHeadMode mod\n");
		return "Failed to init BigHeadMode mod";
	}

	// DevilMayCry4_DX9.exe+5FC978 
	if (!install_hook_offset(0x5FC978 , hook2, &joint_size_detour2, &joint_size_detour2_continue, 0x21)) {
		spdlog::error("Failed to init BigHeadMode mod\n");
		return "Failed to init BigHeadMode mod";
	}
#endif

	return Mod::on_initialize();
}

void BigHeadMode::on_gui_frame(int display) {
	if (display == DISPLAY_NERO_A) {
		if (ImGui::Checkbox(_("Big Head Mode"), &big_head_mode_nero)) {
			swole_mode_nero = false;
		}
		ImGui::SameLine();
		help_marker(_("Head size scales with style"));
		ImGui::SameLine(sameLineWidth);
		if (ImGui::Checkbox(_("Swole Mode"), &swole_mode_nero)) {
			big_head_mode_nero = false;
		}
		ImGui::SameLine();
		help_marker(_("Body size scales with style"));
	}
	if (display == DISPLAY_DANTE_A) {
		if (ImGui::Checkbox(_("Big Head Mode"), &big_head_mode_dante)) {
			swole_mode_dante = false;
		}
		ImGui::SameLine();
		help_marker(_("Head size scales with style"));
		ImGui::SameLine(sameLineWidth);
		if (ImGui::Checkbox(_("Swole Mode"), &swole_mode_dante)) {
			big_head_mode_dante = false;
		}
		ImGui::SameLine();
		help_marker(_("Body size scales with style"));
	}
}
