#include "../mods.h"
#include "modRgMultiplier.hpp"

bool RgMultiplier::modEnabled{ false };
uintptr_t RgMultiplier::jmp_ret{ NULL };
uintptr_t RgMultiplier::jmp_ret2{ NULL };

static float rgmultfloat{ 3.3333f };

naked void detour(void)
{
    _asm {
        cmp byte ptr [RgMultiplier::modEnabled], 0
        je originalcode

        mulss xmm0, [rgmultfloat]

        originalcode:
        addss xmm1, xmm0
        movss [esi+00014DACh], xmm1
        jmp dword ptr [RgMultiplier::jmp_ret]
    }
}

naked void detour2(void) {
    _asm {
        cmp byte ptr [RgMultiplier::modEnabled], 0
        je originalcode

        mulss xmm0, [rgmultfloat]

        originalcode:
        addss xmm1, xmm0
        movss [esi+00014DACh], xmm1
        jmp dword ptr [RgMultiplier::jmp_ret2]
    }
}
std::optional<std::string> RgMultiplier::onInitialize() {
    if (!install_hook_offset(0x03BB973, hook, &detour, &jmp_ret, 12)) {
        HL_LOG_ERR("Failed to init RgMultiplier mod\n");
        return "Failed to init RgMultiplier mod";
    }
    if (!install_hook_offset(0x03BBEDC, hook2, &detour2, &jmp_ret2, 12)) {
        HL_LOG_ERR("Failed to init RgMultiplier mod 2\n");
        return "Failed to init RgMultiplier mod 2";
    }
    return Mod::onInitialize();
}

void RgMultiplier::onGUIframe() {
    ImGui::Checkbox("Increased RG Gain", &modEnabled);
    ImGui::SameLine();
    HelpMarker("Intended to match DMC3's meter gain");
}

void RgMultiplier::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("rg_multiplier").value_or(false);
}

void RgMultiplier::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("rg_multiplier", modEnabled);
}
