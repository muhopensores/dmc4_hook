
#include "ChargeChecker.hpp"

bool ChargeChecker::mod_enabled{ false };
uintptr_t ChargeChecker::jmp_ret{ NULL };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

static float round_trip_charge_mult{ 2.0f };

naked void detour(void) { // player in edi 
    _asm {
        cmp byte ptr [ChargeChecker::mod_enabled], 0
        je code

        push edx
        mov edx, [static_mediator_ptr] // only get player (thanks boss dante)
        mov edx, [edx]
        mov edx, [edx+0x24]
        lea edx, [edx+0x23E8]
        cmp edx, ecx // 0x23D8 == Round Trip charge
        pop edx
        jne code
        cmp dword ptr [edi+0x1494], 0 // controller id
        jne code
        cmp dword ptr [edi+0x1DB4], 4 // rebellion
        jne code

        mulss xmm0, [round_trip_charge_mult]

    code:
        addss xmm0, [ecx+0x10]
        jmp dword ptr [ChargeChecker::jmp_ret]
    }
}

std::optional<std::string> ChargeChecker::on_initialize() {
    if (!install_hook_offset(0x0405F97, hook, &detour, &jmp_ret, 5)) {
        spdlog::error("Failed to init ChargeChecker mod\n");
        return "Failed to init ChargeChecker mod";
    }
    return Mod::on_initialize();
}

void ChargeChecker::on_gui_frame() {
    ImGui::Checkbox("Faster Round Trip Charge", &mod_enabled);
    ImGui::SameLine();
    help_marker("Halves the charge time on Round Trip");
}

void ChargeChecker::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("faster_roundtrip").value_or(false);
};

void ChargeChecker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("faster_roundtrip", mod_enabled);
};
