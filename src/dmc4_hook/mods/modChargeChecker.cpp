#include "../mods.h"
#include "modChargeChecker.hpp"

bool ChargeChecker::modEnabled{ false };
uintptr_t ChargeChecker::jmp_ret{ NULL };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

float roundTripChargeMult{ 2.0f };

naked void detour(void) // player in edi
{
    _asm {
        cmp byte ptr [ChargeChecker::modEnabled], 0
        je code

        cmp eax, 1 // melee input
        jne code

        push edx
        mov edx, [staticMediatorPtr] // only get player (thanks boss dante)
        mov edx, [edx]
        mov edx, [edx+0x24]
        cmp edx, edi
        pop edx
        jne code
        cmp dword ptr [edi+0x1494], 0 // controller id
        jne code
        cmp dword ptr [edi+0x1DB4], 4 // rebellion
        jne code

        mulss xmm0, [roundTripChargeMult]

    code:
        addss xmm0, [ecx+0x10]
        jmp dword ptr [ChargeChecker::jmp_ret]
    }
}

std::optional<std::string> ChargeChecker::onInitialize()
{
    if (!install_hook_offset(0x0405F97, hook, &detour, &jmp_ret, 5))
    {
        HL_LOG_ERR("Failed to init ChargeChecker mod\n");
        return "Failed to init ChargeChecker mod";
    }
    return Mod::onInitialize();
}

void ChargeChecker::onGUIframe()
{
    ImGui::Checkbox("Faster Round Trip", &modEnabled);
}

void ChargeChecker::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("faster_roundtrip").value_or(false);
};

void ChargeChecker::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("faster_roundtrip", modEnabled);
};
