#include "GuardSteer.hpp"

#if 1
bool GuardSteer::mod_enabled{false};

uintptr_t GuardSteer::guard_steer_continue{NULL};

static float startX;
static float startY;
// static float stickDirection;
static float stickDirectionAdjusted;
static float newX;
static float newY;

void LetsNotTryThisInAsm(void) {
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

        // I don't think a grounded inertia direction value exists??
        // For air you can just feed an xmm stick value
        movss [startY], xmm0
        movss [startX], xmm1

        // if people start getting random teles to credo, back up more xmm / movsd
        push ebp
        mov ebp, esp
        sub esp, 3*4
        movss [esp], xmm2
        movss [esp+0x4], xmm3
        movss [esp+0x8], xmm4

        movss xmm5, [eax+0x00001420]
        movss [stickDirectionAdjusted], xmm5

        pushad
        call LetsNotTryThisInAsm
        popad

        movss xmm0, [newY]
        movss xmm1, [newX]
        movss xmm2, [esp]
        // restore xmms
        movss xmm3, [esp+0x4]
        movss xmm4, [esp+0x8]
        mov esp, ebp
        pop ebp

        originalcode:
        comiss xmm4,xmm3
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
    ImGui::Checkbox(_("Guardsteer"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("See your body into the moonlight\n"
                "Even if I try to cancel\n"
                "All the pictures into the mind\n"
                "There's a flashing in my eyes"));
}

void GuardSteer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("guardsteer_enabled").value_or(false);
}

void GuardSteer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("guardsteer_enabled", mod_enabled);
}

#endif
