#include "AngelSnatch.hpp"

bool AngelSnatch::mod_enabled{false};
uintptr_t AngelSnatch::jmp_ret1{false};
constexpr uintptr_t detour1_call1 = 0x007FA190;

uintptr_t AngelSnatch::jmp_ret2{false};
constexpr uintptr_t detour2_call1 = 0x007F9FA0;
uintptr_t AngelSnatch::jmp_ret3{false};
bool snatchFlag = false;

naked void detour1(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push ecx
            mov ecx, [eax+0x21CC]
            test cl, cl
            je handle
            mov byte ptr [snatchFlag], 1
            pop ecx
            jmp originalcode

        handle:
            mov byte ptr [snatchFlag], 0
            pop ecx
		originalcode:
            push eax
            call detour1_call1
        // jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret1]
    }
}

naked void detour2(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push ecx
            mov ecx, [eax+0x21CC]
            test cl, cl
            je handle
            mov byte ptr [snatchFlag], 1
            pop ecx
            jmp originalcode

        handle:
            mov byte ptr [snatchFlag], 0
            pop ecx
		originalcode:
            push eax
            call detour2_call1
        // jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret2]
    }
}

naked void detour3(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push eax
            mov al, [snatchFlag]
            test al,al
            je handle
            pop eax
            pop ecx
            pop ebx
            pop eax
            mov eax, 0x4
            mov byte ptr [snatchFlag],0
            jmp code

        handle:
            pop eax
        originalcode:
            call edx
        code:
            mov edi,eax
            cmp edi, 3
        // jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret3]
    }
}

std::optional<std::string> AngelSnatch::on_initialize() {
    if (!install_hook_offset(0x3F9CF2, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init AngelSnatch mod\n");
        return "Failed to init AngelSnatch mod";
    }
    if (!install_hook_offset(0x3F9CEB, hook2, &detour2, &jmp_ret2, 6)) {
        spdlog::error("Failed to init AngelSnatch mod2\n");
        return "Failed to init AngelSnatch mod2";
    }
    if (!install_hook_offset(0x334F9D, hook3, &detour3, &jmp_ret3, 7)) {
        spdlog::error("Failed to init AngelSnatch mod3\n");
        return "Failed to init AngelSnatch mod3";
    }
    return Mod::on_initialize();
}

void AngelSnatch::on_gui_frame() {
    ImGui::Checkbox("Angel Snatch", &mod_enabled);
    ImGui::SameLine();
    help_marker("Mapped to forward + snatch");
}

void AngelSnatch::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("angel_snatch").value_or(false);
}

void AngelSnatch::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("angel_snatch", mod_enabled);
}
