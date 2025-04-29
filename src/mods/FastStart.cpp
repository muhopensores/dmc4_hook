#include "FastStart.hpp"

static bool mod_enabled = false;
static bool skip_fades = false;
static bool skip_more_fades = false;

static uintptr_t jmp_ret = NULL;
static uintptr_t jmp_ret_u_fade = NULL;

constexpr float fade_amount = 999.0f;

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

		movss xmm0, dword ptr [fade_amount]
		movss dword ptr [esi+0x24], xmm0
		movss dword ptr [esi+0x1C], xmm0
		pop edi
		pop esi
		add esp, 10h
		ret
	
	originalCode:
		movss dword ptr [esi+0x24], xmm0
		movss xmm0, dword ptr [esi+0x1C]
		addss xmm0, dword ptr [esp+0x0C]
		movss dword ptr [esi+0x1C],xmm0
		pop edi
		pop esi
		add esp,10h
		ret
	}
}

std::optional<std::string> FastStart::on_initialize() {

#if 0 // DIDNT WORK LMAOO
	// invalidate shaderlog.sig filename
	static constexpr uintptr_t slog_ptr = 0x00C0070C;
	static constexpr size_t slog_strlen = 14;
	auto old_protect = Patch::protect(slog_ptr, slog_strlen, PAGE_EXECUTE_READWRITE);
	if (old_protect) {
		memset((void*)slog_ptr, 0, slog_strlen);
		Patch::protect(slog_ptr, slog_strlen, old_protect.value());
	}
#else
	if (DeleteFileA("nativePC\\shaderlog.slg")) {
		DISPLAY_MESSAGE("[FastStart]: Deleted shaderlog.sig for faster loads");
	}
#endif


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

void FastStart::on_gui_frame(int display) {
	ImGui::Checkbox(_("Fast Game Load"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("The game will skip all opening screens and load the first save slot"));
    ImGui::SameLine(sameLineWidth);
	ImGui::Checkbox(_("Fast Menu Fades"), &skip_fades);
    ImGui::SameLine();
    help_marker(_("Skip the fades between some menus"));

    if (ImGui::Checkbox(_("Fast BP Fade"), &skip_more_fades)) {
        toggle(skip_more_fades);
    }
    ImGui::SameLine();
    help_marker(_("Skips the BP load fade"));
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
