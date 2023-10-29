
#include "EasyJc.hpp"
#include "FreeJc.hpp"

#if 1
bool EasyJc::mod_enabled{false};
bool EasyJc::mod_enabled2{ true };

uintptr_t EasyJc::easy_jc_continue{ NULL };
uintptr_t EasyJc::easy_jc_continue2{NULL};
static float jc_scale{1.5f};

naked void easy_jc_proc(void) {
    _asm {
		cmp byte ptr [EasyJc::mod_enabled], 0
        je originalcode

        movss xmm3, dword ptr [esi-0x10]
        mulss xmm3, dword ptr [jc_scale]
        jmp dword ptr [EasyJc::easy_jc_continue]

		originalcode:
        movss xmm3, dword ptr [esi-0x10]
		jmp dword ptr [EasyJc::easy_jc_continue]
    }
}

// this cheat will be moved i hope
#include <cmath>
static float startX;
static float startY;
//static float stickDirection;
static float stickDirectionAdjusted;
static float newX;
static float newY;

void LetsNotTryThisInAsm(void) {;
    float newDirectionRadians = stickDirectionAdjusted;
    float magnitude = std::sqrt(startX * startX + startY * startY);
    newX = magnitude * std::cos(newDirectionRadians);
    newY = magnitude * std::sin(newDirectionRadians);
}

naked void easy_jc_proc2(void) {
    _asm {
        // there is no reason for this to be in easy jc
		cmp byte ptr [EasyJc::mod_enabled2], 0
        je originalcode
        cmp dword ptr [eax+0x00002998], 0x132
        jne originalcode
        cmp dword ptr [eax+0x0000141C], 0 // stick threshold
        je originalcode

        // I don't think a grounded inertia direction value exists??
        // For air you can just feed an xmm stick value
        movss [startY], xmm0
        movss [startX], xmm1

        // if people start getting random teles to credo, back up more xmms
        push ebp
        mov ebp, esp
        sub esp, 3*4
        movss [esp], xmm2
        movss [esp+0x4], xmm3
        movss [esp+0x8], xmm4
        // movss [esp+0x28], xmm5
        // movss [esp+0x30], xmm6
        // movss [esp+0x38], xmm7

        // movss xmm5, [eax+0x000013C4]
        // movss [stickDirection] , xmm5
        movss xmm5, [eax+0x00001420]
        movss [stickDirectionAdjusted], xmm5

        pushad
        call LetsNotTryThisInAsm
        popad

        movss xmm0, [newY]
        movss xmm1, [newX]
        movss xmm2, [esp]
        movss xmm3, [esp+0x4]
        movss xmm4, [esp+0x8]
        // movss xmm5, [esp+0x28]
        // movss xmm6, [esp+0x30]
        // movss xmm7, [esp+0x38]
        mov esp, ebp
        pop ebp

        originalcode:
        comiss xmm4,xmm3 // whoops
        movss [eax+0x00000EC0], xmm0
		jmp dword ptr [EasyJc::easy_jc_continue2]
    }
}
/* // This was actual steering
    void LetsDoThisInC(void) {
        newX = startX * cos(playerfacing) + startY * sin(playerfacing);
        newY = startX * sin(playerfacing) - startY * cos(playerfacing);
    }

    naked void easy_jc_proc2(void) {
        _asm {
            //
		    cmp byte ptr [EasyJc::mod_enabled2], 0
            je originalcode

            movss [startY], xmm0
            movss [startX], xmm1
            movss [xmm5backup], xmm5
            movss xmm5, [eax+0x000013C4] // StickFacing
            // movss xmm5, [eax+0x00001420] // StickFacing adjusted to camera
            movss [playerfacing] , xmm5
            
            movss [xmm2backup], xmm2
            movss [xmm3backup], xmm3
            movss [xmm4backup], xmm4
            movss [xmm6backup], xmm6
            movss [xmm7backup], xmm7

            pushad
            call letsdothisinc
            popad

            movss xmm0, [newY]
            movss xmm1, [newX]
            movss xmm2, [xmm2backup]
            movss xmm3, [xmm3backup]
            movss xmm4, [xmm4backup]
            movss xmm5, [xmm5backup] 
            movss xmm6, [xmm6backup]
            movss xmm7, [xmm7backup] 


        originalcode:
            movss [eax+0x00000EC0], xmm0
		    jmp dword ptr [EasyJc::easy_jc_continue2]
        }
    }
*/

std::optional<std::string> EasyJc::on_initialize() {
    if (!install_hook_offset(0xAB336, hook, &easy_jc_proc, &EasyJc::easy_jc_continue, 5)) {
        spdlog::error("Failed to init EasyJc mod\n");
        return "Failed to init EasyJc mod";
    }
    if (!install_hook_offset(0x3ACF3C, hook2, &easy_jc_proc2, &EasyJc::easy_jc_continue2, 8)) {
        spdlog::error("Failed to init EasyJc mod\n");
        return "Failed to init EasyJc mod";
    }


    return Mod::on_initialize();
}

void EasyJc::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x427999, patch, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        install_patch_offset(0x427999, patch, "\xBF\x02\x00\x00\x00", 5);
    }
}

void EasyJc::on_gui_frame() {
    if (ImGui::Checkbox("Easy Enemy Step", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("50% bigger Enemy Step hitspheres");
    if (ImGui::Checkbox("Guardsteer", &mod_enabled2)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("See your body into the moonlight\n"
                "Even if I try to cancel\n"
                "All the pictures into the mind\n"
                "There's a flashing in my eyes");
}

void EasyJc::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("easy_jc").value_or(false);
    toggle(mod_enabled);
}

void EasyJc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("easy_jc", mod_enabled);
}

#endif
