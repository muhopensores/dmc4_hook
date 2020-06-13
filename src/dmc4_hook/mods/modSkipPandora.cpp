#include "../mods.h"
#include "modSkipPandora.hpp"

#if 1

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
    if (!install_hook_offset(0x2E844B, hook, &skipPandora_proc, &SkipPandora::_skipPandoraContinue, 5))
    {
        HL_LOG_ERR("Failed to init SkipPandora mod\n");
        return "Failed to init SkipPandora mod";
    }

    return Mod::onInitialize();
}

void SkipPandora::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
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
