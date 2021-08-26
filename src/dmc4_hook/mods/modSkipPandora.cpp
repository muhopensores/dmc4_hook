#include "../mods.h"
#include "modSkipPandora.hpp"

#if 1
static bool hooked{ false };

bool SkipPandora::modEnabled{ false };
uintptr_t SkipPandora::_skipPandoraContinue{ NULL };

SkipPandora::SkipPandora()
{
	// onInitialize();
}

naked void skipPandora_proc(void)
{
	_asm {
		cmp byte ptr [SkipPandora::modEnabled], 0
		je originalcode

		cmp eax, 8
		je skiptoei
		jmp originalcode

		skiptoei:
		mov [edi+30h], 9
			mov al, 01
			jmp dword ptr [SkipPandora::_skipPandoraContinue]

			originalcode:
			mov [edi+30h], eax
			mov al, 01
			jmp dword ptr [SkipPandora::_skipPandoraContinue]
	}
}

std::optional<std::string> SkipPandora::onInitialize()
{
	// bad fix for crashing on securom exe
	const unsigned char bytes[5] = { 0x89, 0x47, 0x30, 0xB0, 0x01 };
	if (memcmp((void*)0x7B122F, bytes, sizeof(bytes))) {
		hooked = false;
		return Mod::onInitialize();
	}
	hooked = true;
	if (!install_hook_offset(0x3B122F, hook, &skipPandora_proc, &SkipPandora::_skipPandoraContinue, 5)) // breaks 28mb exe
	{
		HL_LOG_ERR("Failed to init SkipPandora mod\n");
		return "Failed to init SkipPandora mod";
	}

	return Mod::onInitialize();
}

void SkipPandora::onGUIframe()
{
	if (!hooked) { return; }
	ImGui::Checkbox("Skip Pandora", &modEnabled);
}

void SkipPandora::onConfigLoad(const utils::Config& cfg)
{
	modEnabled = cfg.get<bool>("skip_pandora").value_or(false);
};

void SkipPandora::onConfigSave(utils::Config& cfg)
{
	cfg.set<bool>("skip_pandora", modEnabled);
};

#endif
