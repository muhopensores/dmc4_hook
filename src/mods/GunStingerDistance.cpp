#include "GunStingerDistance.hpp"

bool      GunStingerDistance::mod_enabled{ false };
uintptr_t GunStingerDistance::jmp_ret{ NULL };

static constexpr float gunStingerSpeed = 16.0f;
static constexpr float gunStingerDistance = 560.0f;

naked void detour(void) {
	_asm {
		cmp byte ptr [GunStingerDistance::mod_enabled], 0
		je originalcode

		movss xmm0,[gunStingerSpeed]
		movss [ebx+0x0000177C],xmm0
		mov eax,[ebx+0x00001EA8]
		movss xmm1,[gunStingerDistance]
		jmp retcode

		originalcode:
		movss xmm0,[edx+0x00000210]
		movss [ebx+0x0000177C],xmm0
		mov eax,[ebx+0x00001EA8]
		movss xmm1,[eax+0x00000214]
		retcode:
		jmp dword ptr [GunStingerDistance::jmp_ret]
	}
}

std::optional<std::string> GunStingerDistance::on_initialize() {
	if (!install_hook_offset(0x3DB1EA, hook, &detour, &jmp_ret, 30)) {
		spdlog::error("Failed to init GunStingerDistance mod\n");
		return "Failed to init GunStingerDistance mod";
	}
	return Mod::on_initialize();
}

void GunStingerDistance::on_gui_frame() {
	ImGui::Checkbox(_("Gun Stinger 2"), &mod_enabled);
    ImGui::SameLine();
	help_marker(_("Gun Stinger covers the same distance as Stinger 2"));
}

void GunStingerDistance::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("gunstinger_two").value_or(false);
};

void GunStingerDistance::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("gunstinger_two", mod_enabled);
};
