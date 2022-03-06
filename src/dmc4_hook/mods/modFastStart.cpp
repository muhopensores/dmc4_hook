#include "../mods.h"
#include "modFastStart.hpp"

static bool modEnabled{ false };
static bool skipFades { false };

static uintptr_t jmp_ret;
static uintptr_t jmp_ret_uFade;

constexpr float fadeAmmount = 999.f;

static bool checkClassName(MtDTI* dti) {
	char* aTitle = "aInit";
	return strcmp(dti->mName, aTitle) == 0;
}

static void loadSaveSlot() {
	uintptr_t func = 0x004A03B0; // load function ptr
	_asm {
		mov edx, 0x00E55E48 // sPCSave
		mov edx, [edx]
		mov eax, 0 // slot?
		push edx
		call dword ptr [func]
	}
}

naked void detour(void)
{
	_asm {
		cmp byte ptr[modEnabled], 0
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
		call loadSaveSlot //load save in slot 0
		popad

	originalCode:

		mov edx, [ecx]
		mov eax, [edx+4h]

		jmp dword ptr [jmp_ret]
	}
}

naked void detour_uFade() {
	
	__asm {
		cmp byte ptr[skipFades], 0
		je originalCode

		movss xmm0, dword ptr [fadeAmmount]
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

std::optional<std::string> FastStart::onInitialize()
{
	install_hook_absolute(0x008DB77D, hook_dti, detour, &jmp_ret, 5);
	install_hook_absolute(0x00739632, hook_uFade, detour_uFade, &jmp_ret_uFade, 5);

	return Mod::onInitialize();
}


void FastStart::onGUIframe()
{
	ImGui::Checkbox("Fast Game Load", &modEnabled);
    ImGui::SameLine(205);
	ImGui::Checkbox("Fast Menu Fades", &skipFades);
}

void FastStart::onConfigLoad(const utils::Config& cfg)
{
	modEnabled = cfg.get<bool>("fast_load").value_or(false);
	skipFades  = cfg.get<bool>("skip_fades").value_or(false);
};

void FastStart::onConfigSave(utils::Config& cfg)
{
	cfg.set<bool>("fast_load", modEnabled);
	cfg.set<bool>("skip_fades", skipFades);
};
