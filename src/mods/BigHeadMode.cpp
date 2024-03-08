#include "BigHeadMode.hpp"

#include "glm/gtx/compatibility.hpp"
#include "../sdk/Devil4.hpp"

bool g_enable_mod = false;
bool g_swole_mode = false;
static uintptr_t joint_size_detour1_continue = NULL;
static uintptr_t joint_size_detour2_continue = NULL;

glm::vec3 size{ 0.5f, 0.5f, 0.5f };
glm::vec3 size_torso{ 0.9f, 0.8f, 0.8f };


static void scale_head_joint(UModelJoint* joint) {
	if (g_swole_mode) {
		joint->size = size * (glm::clamp(devil4_sdk::get_current_style_rank(), 1.5f, 3.0f));
	}
	else {
		joint->size = size * devil4_sdk::get_current_style_rank();
	}
}

static int is_head_joint(UModelJoint* joint) {
	uPlayer* u_plr = devil4_sdk::get_local_player();
    if (u_plr) {
        if (g_swole_mode) {
            UModelJoint* torso = &u_plr->joint_array->joint[2]; // seems to be torso for both chars
            return joint == torso;
        }
        UModelJoint* head = &u_plr->joint_array->joint[4]; // seems to be heads for both chars
        return joint == head;
	}
    return 0;
}

naked void joint_size_detour1() {
	__asm {
		pushf
		cmp byte ptr [g_enable_mod], 1
		jne originalCode1
		pusha
		push esi
		call is_head_joint
		test eax, eax
		pop esi
		popa
	    je originalCode1
		pusha
		push esi
		call scale_head_joint
		pop esi
		popa
		popf
		jmp bail
	originalCode1:
		popf
		movss [esi+0x30],xmm0
		movss xmm0,[esp+0x54]
		movss [esi+0x34],xmm0
		movss xmm0,[esp+0x58]
		movss [esi+0x38],xmm0
	bail:
		jmp dword ptr [joint_size_detour1_continue]
	}
}

naked void joint_size_detour2() {
	__asm {
		pushf
		cmp byte ptr [g_enable_mod], 1
		jne originalCode2
		pusha
		push esi
		call is_head_joint
		test eax, eax
		pop esi
		popa
		je originalCode2
		pusha
		push esi
		call scale_head_joint
		pop esi
		popa
		popf
		jmp bail

	originalCode2:
		popf
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
        g_enable_mod = !g_enable_mod;
    }).set_timer(300.0, []() {
        g_enable_mod = false;
    });

    MutatorRegistry::define("SwoleMode")
        .on_init( []() {
        g_enable_mod = !g_enable_mod;
        g_swole_mode = true;
    }).set_timer(300.0, []() {
        g_enable_mod = false;
        g_swole_mode = false;
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

void BigHeadMode::on_gui_frame() {
    if (ImGui::Checkbox("Big Head Mode", &g_enable_mod)) {
        if (g_swole_mode == true)
            g_swole_mode = false;
	}
    ImGui::SameLine(sameLineWidth);
	if (ImGui::Checkbox("Swole Mode", &g_swole_mode)) {
		g_enable_mod = g_swole_mode;
	}
}