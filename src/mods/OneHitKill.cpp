#include "OneHitKill.hpp"

bool OneHitKill::cant_die = false;
bool OneHitKill::one_hit_kill = false;
uintptr_t OneHitKill::jmp_ret = NULL;
uintptr_t OneHitKill::jmp_out = 0x0051C129;
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

static int one_hit_kill_hotkey;

void OneHitKill::no_death_toggle(bool enable){ // no death 
    if (enable) {
        //install_patch_offset(0x11C11B, patchhp, "\xEB", 1); // used for both
        install_patch_offset(0x11C694, patchomen, "\xEB", 1);
    }
    else {
        //patchhp.reset();
        patchomen.reset();
    }
}

naked void detour(void) {
    _asm {
            cmp byte ptr [OneHitKill::one_hit_kill], 1
            je playercheck
			cmp byte ptr [OneHitKill::cant_die], 1
			je jmpout
        code:
			comiss xmm2, [esi+0x18]
			jb jmpout
        jmpret:
			jmp dword ptr [OneHitKill::jmp_ret]

		jmpout:
			jmp dword ptr [OneHitKill::jmp_out]

        playercheck:
            push ecx
            push edx
            mov ecx, [static_mediator_ptr]
            mov ecx, [ecx]
            mov ecx, [ecx+0x24]
            mov edx, [esi+0xC0]
            cmp edx, ecx
            pop edx
            pop ecx
            je code
            jmp jmpret
    }
}

std::optional<std::string> OneHitKill::on_initialize() {
    if (!install_hook_offset(0x011C117, hook, &detour, &jmp_ret, 6)) {
        spdlog::error("Failed to init OneHitKill mod\n");
        return "Failed to init OneHitKill mod";
    }

    MutatorRegistry::define("OneHitKill")
        .description("hehe")
        .on_init([&] { one_hit_kill = true; })
        .set_timer(15.0, [&] { one_hit_kill = false; });

    console->system().RegisterCommand("onehitkill", "Enables one hit kill", [this]() {
        OneHitKill::one_hit_kill = !OneHitKill::one_hit_kill;
    });    

    utility::create_keyboard_hotkey(OneHitKill::m_hotkeys, {VK_F3}, "One Hit Kill", "one_hit_kill_key");

    return Mod::on_initialize();
}

void OneHitKill::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("No Death (All)"), &cant_die)) {
        one_hit_kill = false;
        //toggle2(oneHitKill);
        no_death_toggle(cant_die);
    }
    ImGui::SameLine();
    help_marker(_("Disables any entity dying while still allowing HP to drop.\nFor practicing against enemies who have AI changes at "
                "different levels of HP\nAlso stops Omen killing enemies."));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("One Hit Kill"), &one_hit_kill)) {
        cant_die = false;
        no_death_toggle(cant_die);
        //toggle2(oneHitKill);
    }
    ImGui::SameLine();
    help_marker(_("Enables one hit kill. This overrides the damage multiplier. By default this is hotkeyed to F3"));
}

void OneHitKill::on_update_input(utility::Input& input) {
    if (OneHitKill::m_hotkeys[0]->check(input)) {
        if (one_hit_kill) {
            DISPLAY_MESSAGE("One Hit Kill Off");
        }
        else {
            DISPLAY_MESSAGE("One Hit Kill On");
            cant_die = false;
            no_death_toggle(cant_die);
        }
        one_hit_kill = !one_hit_kill;
        // toggle2(oneHitKill);
    }
}


void OneHitKill::on_config_load(const utility::Config& cfg) {
    cant_die = cfg.get<bool>("no_death").value_or(false);
    one_hit_kill = cfg.get<bool>("one_hit_kill").value_or(false);
    no_death_toggle(cant_die);
}

void OneHitKill::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_death", cant_die);
    cfg.set<bool>("one_hit_kill", one_hit_kill);
}
