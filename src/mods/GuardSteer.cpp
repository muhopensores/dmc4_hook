#include "GuardSteer.hpp"

#if 1
bool GuardSteer::mod_enabled = false;

uintptr_t GuardSteer::guard_steer_continue = NULL;

static float startX;
static float startY;
// static float stickDirection;
static float stickDirectionAdjusted;
static float newX;
static float newY;

static void LetsNotTryThisInAsm() {
    float magnitude           = std::sqrt(startX * startX + startY * startY);
    newX                      = magnitude * std::cos(stickDirectionAdjusted);
    newY                      = magnitude * std::sin(stickDirectionAdjusted);
}

naked void guard_steer_proc(void) {
    _asm {
		cmp byte ptr [GuardSteer::mod_enabled], 0
        je originalcode
        cmp dword ptr [eax+0x00001494], 0 // controller id dante
        jne originalcode
        cmp dword ptr [eax+0x00002998], 0x132 // moveid, guard
        jne originalcode
        cmp dword ptr [eax+0x0000141C], 0 // stick threshold
        je originalcode

        // if people start getting random teles to credo, back up more xmm / movsd
        sub esp, 6*0x10
        movups [esp+0*0x10], xmm2
        movups [esp+1*0x10], xmm3
        movups [esp+2*0x10], xmm4
        movups [esp+3*0x10], xmm5
        movups [esp+4*0x10], xmm6
        movups [esp+5*0x10], xmm7

        // I don't think a grounded inertia direction value exists??
        // For air you can just feed an xmm stick value
        movss [startY], xmm0
        movss [startX], xmm1

        movss xmm5, [eax+0x00001420]
        movss [stickDirectionAdjusted], xmm5

        pushad
        call LetsNotTryThisInAsm
        popad

        // restore xmms
        movups xmm7, [esp+5*0x10]
        movups xmm6, [esp+4*0x10]
        movups xmm5, [esp+3*0x10]
        movups xmm4, [esp+2*0x10]
        movups xmm3, [esp+1*0x10]
        movups xmm2, [esp+0*0x10]
        add esp, 6*0x10

        movss xmm0, [newY]
        movss xmm1, [newX]

        originalcode:
        comiss xmm4, xmm3
        movss [eax+0x00000EC0], xmm0
		jmp dword ptr [GuardSteer::guard_steer_continue]
    }
}

/*void LetsDoThisInC(void) { // This was actual steering 
    newX = startX * cos(stickDirection) + startY * sin(stickDirection);
    newY = startX * sin(stickDirection) - startY * cos(stickDirection);
}*/

std::optional<std::string> GuardSteer::on_initialize() {
    if (!install_hook_offset(0x3ACF3C, hook, &guard_steer_proc, &GuardSteer::guard_steer_continue, 8)) {
        spdlog::error("Failed to init GuardSteer mod\n");
        return "Failed to init GuardSteer mod";
    }
    return Mod::on_initialize();
}

void GuardSteer::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Guardsteer"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("See your body into the moonlight\n"
                      "Even if I try to cancel\n"
                      "All the pictures into the mind\n"
                      "There's a flashing in my eyes"));
    }
}

void GuardSteer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("guardsteer_enabled").value_or(false);
}

void GuardSteer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("guardsteer_enabled", mod_enabled);
}

#endif
