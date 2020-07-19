#include "modKnockbackEdits.hpp"
#if 1

bool KnockbackEdits::modEnabled{ false };
uintptr_t KnockbackEdits::jmp_return{ NULL };
constexpr auto shottySwap =
    "4E 52 4D 4C 5F 45 54 43 5F 4C 56 31 00 00 00 00"
    "2C 00 00 00 00 00 70 41 00 00 00 00 01 00 00 00"
    "02 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00" // First byte selects if it should work when grounded // Originally 1
    "00 00 00 00 04 00 00 00 52 00 00 00 0B 00 00 00" // Fifth byte selects knockback type // Originally 2
    "52 00 00 00 0B 00 00 00 00 00 C8 41 00 00 16 43"
    "00 00 B4 41 00 00 00 00 01 00 00 00 00 00 00 00";

KnockbackEdits::KnockbackEdits()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
		cmp byte ptr [KnockbackEdits::modEnabled], 0
		je originalcode

        cmp dword ptr [edx], 1280135758 // 'NRML_ETC' // Shotty
        je newmove
        jmp originalcode

    newmove:
        mov edx, dword ptr shottySwap
        jmp originalcode

    originalcode:
        mov esi, edx // edx = string of move at the start of info
        repe movsd // esi incs 4 every repe movsd starting at edx
        pop edi
		jmp dword ptr [KnockbackEdits::jmp_return]
    }
}

std::optional<std::string> KnockbackEdits::onInitialize()
{
    /*if (!install_hook_offset(0x1099F8, hook, &detour, &jmp_return, 5))
    {
        HL_LOG_ERR("Failed to init KnockbackEdits mod\n");
        return "Failed to init KnockbackEdits mod";
    }*/

    return Mod::onInitialize();
}

void KnockbackEdits::onGUIframe()
{
    // ImGui::Checkbox("Knockback Edits", &modEnabled);
}

void KnockbackEdits::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("knockback_edits").value_or(false);
};

void KnockbackEdits::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("knockback_edits", modEnabled);
};

#endif
