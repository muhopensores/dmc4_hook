
#include "FastStart.hpp"

static bool mod_enabled{ false };
static bool skip_fades { false };
static bool skip_more_fades{ false };

static uintptr_t jmp_ret;
static uintptr_t jmp_ret_u_fade;

constexpr float fade_ammount = 999.f;

static bool check_class_name(MtDTI* dti) {
	char* a_title = "aInit";
	return strcmp(dti->m_name, a_title) == 0;
}

static void load_save_slot() {
	uintptr_t func = 0x004A03B0; // load function ptr
	_asm {
		mov edx, 0x00E55E48 // sPCSave
		mov edx, [edx]
		mov eax, 0 // slot?
		push edx
		call dword ptr [func]
	}
}

naked void detour(void) {
	_asm {
		cmp byte ptr[mod_enabled], 0
		je originalCode

		cmp ecx, 0x00E55C88 // aAdvertise
		je aAdv
		cmp ecx, 0x00E55D88 // aInit
		je aInit
		jmp originalCode
	aAdv:
		mov ecx, 0x00E55E48 // aMissionSelect
		jmp originalCode

		; 0x00E55CE8 // aGame
		; 0x00E55C88 // aAdvertise

		; mov ecx, 0x00E56BC8 // aTitle
		; mov ecx, 0x00E55E48  // aMissionSelect
	aInit:
		mov ecx, 0x00E55CE8 // aGame
		pushad
		call load_save_slot //load save in slot 0
		popad

	originalCode:

		mov edx, [ecx]
		mov eax, [edx+4h]

		jmp dword ptr [jmp_ret]
	}
}

naked void detour_u_fade() {
	__asm {
		cmp byte ptr[skip_fades], 0
		je originalCode

		movss xmm0, dword ptr [fade_ammount]
		movss dword ptr [esi+24h], xmm0
		movss dword ptr [esi+1Ch], xmm0
		pop edi
		pop esi
		add esp, 10h
		ret
	
	originalCode:
		movss dword ptr [esi+24h], xmm0
		movss xmm0, dword ptr [esi+1Ch]
		addss xmm0, dword ptr [esp+0Ch]
		movss dword ptr [esi+1Ch],xmm0
		pop edi
		pop esi
		add esp,10h
		ret
	}
}

std::optional<std::string> FastStart::on_initialize() {
	install_hook_absolute(0x008DB77D, hook_dti, detour, &jmp_ret, 5);
	install_hook_absolute(0x00739632, hook_u_fade, detour_u_fade, &jmp_ret_u_fade, 5);

	return Mod::on_initialize();
}

void FastStart::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0xBC96F, bp_fade_patch1, "\x90\x90", 2);
        install_patch_offset(0xBCB91, bp_fade_patch2, "\x90\x90", 2);
    }
    else {
        bp_fade_patch1.reset();
        bp_fade_patch2.reset();
    }
}

void FastStart::on_gui_frame() {
	ImGui::Checkbox("Fast Game Load", &mod_enabled);
    ImGui::SameLine();
    help_marker("The game will skip all opening screens and load the first save slot");
    ImGui::SameLine(205);
	ImGui::Checkbox("Fast Menu Fades", &skip_fades);
    ImGui::SameLine();
    help_marker("Skip the fades between some menus");

    if (ImGui::Checkbox("Fast BP Fade", &skip_more_fades)) {
        toggle(skip_more_fades);
    }
    ImGui::SameLine();
    help_marker("Mostly untested, should skip fade on BP load and nothing else");
}

void FastStart::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("fast_load").value_or(false);
	skip_fades  = cfg.get<bool>("skip_fades").value_or(false);
	skip_more_fades  = cfg.get<bool>("skip_more_fades").value_or(false);
    if (skip_more_fades)
        toggle(skip_more_fades);
}

void FastStart::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("fast_load", mod_enabled);
	cfg.set<bool>("skip_fades", skip_fades);
	cfg.set<bool>("skip_more_fades", skip_more_fades);
}
