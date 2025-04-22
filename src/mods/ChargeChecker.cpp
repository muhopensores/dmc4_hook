#include "ChargeChecker.hpp"

bool ChargeChecker::roundTripCharge_enabled = false;
bool ChargeChecker::blueRoseCharge_enabled = false;
uintptr_t ChargeChecker::jmp_ret{ NULL };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

static constexpr float round_trip_charge_mult = 2.0f;
static constexpr float blue_rose_charge_mult_1 = 2.0f;
static constexpr float blue_rose_charge_mult_2 = 2.0f;

naked void detour(void) { // player in edi 
    _asm {
        cmp byte ptr [ChargeChecker::roundTripCharge_enabled], 1
        je DanteChecks
        jmp NeroChecks

        DanteChecks:
        push eax // keep player in eax
        push edx // put compares in edx
        mov eax, [static_mediator_ptr] // only get player (thanks boss dante)
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp dword ptr [edi+0x1494], 0 // controller id (0 Dante 1 Nero)
        je GotDante
        pop edx
        pop eax
        jmp NeroChecks

        ////////// Dante //////////

        GotDante:
        // cmp dword ptr [edi+0x1DB4], 4 // rebellion
        // je RoundTrip
        jmp RoundTrip

        RoundTrip:
        lea edx, [eax+0x23E8]  // 0x23F8 == Round Trip charge
        cmp edx, ecx // ecx = affected address
        jne popcode
        mulss xmm0, [round_trip_charge_mult]
        jmp popcode

        ////////// Nero //////////

        NeroChecks:
        cmp byte ptr [ChargeChecker::blueRoseCharge_enabled], 0
        je code
        push eax // keep player in eax
        push edx // put compares in edx
        mov eax, [static_mediator_ptr] // only get player (thanks boss dante)
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp dword ptr [edi+0x1494], 1 // Nero
        je GotNero

        GotNero:
        jmp BlueRose
            
        BlueRose:
        // lea edx, [eax+0x2420] // unkn
        // lea edx, [eax+0x2434] // 0
        // lea edx, [eax+0x2448] // unkn
        // lea edx, [eax+0x245C] // unkn
        lea edx, [eax+0x2470]    // 1 to 2
        sub edx, 0x10
        cmp edx, ecx             // ecx = affected address
        je BlueRoseCharge1to2
        // lea edx, [eax+0x2484] // unkn
        lea edx, [eax+0x2498]    // 2 to max
        sub edx, 0x10
        cmp edx, ecx             // ecx = affected address
        je BlueRoseCharge2to3
        // lea edx, [eax+0x24AC] // unkn
        jmp popcode

        BlueRoseCharge1to2:
        mulss xmm0, [blue_rose_charge_mult_1]
        jmp popcode

        BlueRoseCharge2to3:
        mulss xmm0, [blue_rose_charge_mult_2]
        jmp popcode

    popcode:
        pop edx
        pop eax
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

void ChargeChecker::on_gui_frame(int display) {
    if (display == 2) {
        ImGui::Checkbox(_("Fast Round Trip Charge"), &roundTripCharge_enabled);
        ImGui::SameLine();
        help_marker(_("Half the charge time on Round Trip"));
    }
    else if (display == 1) {
        ImGui::Checkbox(_("Fast Blue Rose Charge"), &blueRoseCharge_enabled);
        ImGui::SameLine();
        help_marker(_("Half the charge time on Blue Rose"));
    }
}

void ChargeChecker::on_config_load(const utility::Config& cfg) {
    roundTripCharge_enabled = cfg.get<bool>("faster_roundtrip").value_or(false);
    // round_trip_charge_mult = cfg.get<float>("roundtrip_charge_mult_1").value_or(2.0f);
    blueRoseCharge_enabled = cfg.get<bool>("faster_bluerose").value_or(false);
    // blue_rose_charge_mult_1 = cfg.get<float>("blue_rose_charge_mult_1").value_or(2.0f);
    // blue_rose_charge_mult_2 = cfg.get<float>("blue_rose_charge_mult_2").value_or(2.0f);
};

void ChargeChecker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("faster_roundtrip", roundTripCharge_enabled);
    // cfg.set<float>("roundtrip_charge_mult_1", round_trip_charge_mult);
    cfg.set<bool>("faster_bluerose", blueRoseCharge_enabled);
    // cfg.set<float>("blue_rose_charge_mult_1", blue_rose_charge_mult_1);
    // cfg.set<float>("blue_rose_charge_mult_2", blue_rose_charge_mult_2);
};
