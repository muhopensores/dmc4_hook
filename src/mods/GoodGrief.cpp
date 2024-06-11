#include "GoodGrief.hpp"

#if 1

bool GoodGrief::mod_enabled{ false };

uintptr_t GoodGrief::jmp_ret1{NULL};
uintptr_t GoodGrief::jmp_ret2{NULL};
    constexpr uintptr_t detour2_jmp = 0x836FCF;
void GoodGrief::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x436FAF, patch1, "\x90\x90",2);
        install_patch_offset(0x436FB8, patch2, "\x90\x90",2);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

naked void detour1() {
    _asm {
            cmp byte ptr [GoodGrief::mod_enabled],1
            jne originalcode

            mov byte ptr [ebp+0x3174],2//movement abilities cancel
            mov byte ptr [ebp+0x30C4],2//melee cancel
            mov byte ptr [ebp+0x31CC],2//gun cancel
            mov byte ptr [ebp+0x3148],2//directional melee cancel
            mov byte ptr [ebp+0x30F0],2
        originalcode:
            movss xmm0,[ebp+0x0000177C]
            jmp [GoodGrief::jmp_ret1]
    }
}

naked void detour2() { //janky as all hell, but it works
    _asm {
            cmp byte ptr [GoodGrief::mod_enabled],1
            jne originalcode
            
            cmp dword ptr [eax+0x1564],0x59
            je originalcode

            test word ptr [eax+0x140C],0x2//input held
            jne handler2

            cmp [eax+0x1DB8],8//weapon
            jne handler

            test word ptr [eax+0x140C],0x200//input held
            je handler

            cmp byte ptr [eax+0x14D98],1//style
            jne handler
            
            jmp [GoodGrief::jmp_ret2]

        originalcode:
            cmp dword ptr [eax+0x1564],0x59
            je handler 
            jmp [GoodGrief::jmp_ret2]
        handler:
            jmp [detour2_jmp]
        handler2:
            cmp [eax+0x1DB8],8//weapon
            jne handler
            jmp originalcode

    }
}

std::optional<std::string> GoodGrief::on_initialize() {
    if (!install_hook_offset(0x3DF715, hook1, &detour1, &jmp_ret1, 8)) {
		spdlog::error("Failed to init GoodGrief mod\n");
		return "Failed to init GoodGrief mod";
	}
    if (!install_hook_offset(0x436FB1, hook2, &detour2, &jmp_ret2, 7)) {
		spdlog::error("Failed to init GoodGrief mod\n");
		return "Failed to init GoodGrief mod2";
	}
    return Mod::on_initialize();
}

void GoodGrief::on_gui_frame() {
    if (ImGui::Checkbox(_("Good Grief"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Allows Dante to cancel out of Grief mid-throw animation without recalling Pandora"));
}

void GoodGrief::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("good_grief").value_or(false);
	toggle(mod_enabled);
};

void GoodGrief::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("good_grief", mod_enabled);
};

#endif