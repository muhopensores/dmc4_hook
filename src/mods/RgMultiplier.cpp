
#include "RgMultiplier.hpp"

bool RgMultiplier::mod_enabled{ false };
uintptr_t RgMultiplier::jmp_ret{ NULL };
uintptr_t RgMultiplier::jmp_ret2{ NULL };

static float rgmultfloat{ 3.3333f };

naked void detour(void)
{
    _asm {
        cmp byte ptr [RgMultiplier::mod_enabled], 0
        je originalcode

        mulss xmm0, [rgmultfloat]

        originalcode:
        addss xmm1, xmm0
        movss [esi+0x00014DAC], xmm1
        jmp dword ptr [RgMultiplier::jmp_ret]
    }
}

naked void detour2(void) {
    _asm {
        cmp byte ptr [RgMultiplier::mod_enabled], 0
        je originalcode

        mulss xmm0, [rgmultfloat]

        originalcode:
        addss xmm1, xmm0
        movss [esi+0x00014DAC], xmm1
        jmp dword ptr [RgMultiplier::jmp_ret2]
    }
}
std::optional<std::string> RgMultiplier::on_initialize() {
    if (!install_hook_offset(0x03BB973, hook, &detour, &jmp_ret, 12)) {
        spdlog::error("Failed to init RgMultiplier mod\n");
        return "Failed to init RgMultiplier mod";
    }
    if (!install_hook_offset(0x03BBEDC, hook2, &detour2, &jmp_ret2, 12)) {
        spdlog::error("Failed to init RgMultiplier mod 2\n");
        return "Failed to init RgMultiplier mod 2";
    }

    MutatorRegistry::define("IncreasedRGgain")
        .description("hehe")
        .on_init([&] { mod_enabled = true; })
        .set_timer(35.0, [&] { mod_enabled = false; });

    return Mod::on_initialize();
}

void RgMultiplier::on_gui_frame() {
    ImGui::Checkbox(_("Increased RG Gain"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Intended to match DMC3's Royal Guard meter gain"));
}

void RgMultiplier::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("rg_multiplier").value_or(false);
}

void RgMultiplier::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("rg_multiplier", mod_enabled);
}
