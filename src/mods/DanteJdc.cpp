#include "DanteJdc.hpp"

#if 1
bool DanteJdc::mod_enabled{false};
bool DanteJdc::alt_input_enabled{false};
bool DanteJdc::inertia_enabled{true};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

uintptr_t DanteJdc::jmp_ret1{NULL};
    constexpr uintptr_t detour1_push1 = 0x00486640;
    constexpr uintptr_t detour1_mov1  = 0x00E559D8;
    constexpr uintptr_t detour1_call1 = 0x00A47AA0;
    bool jdcFlag    = 0;
    bool canJdc = 0;
    float jdcXCoord = 0.0f;
    float jdcYCoord = 0.0f;
    float jdcZCoord = 0.0f;
uintptr_t DanteJdc::jmp_ret2{NULL};
    constexpr uintptr_t detour2_call1 = 0x0081F970;
    constexpr uintptr_t detour2_jne   = 0x007E134D;
    float jdcTimer2 = 6.0f;
    float jdcTimer1 = 5.0f;
uintptr_t DanteJdc::jmp_ret3{NULL};
uintptr_t DanteJdc::jmp_ret4{NULL};
    constexpr uintptr_t detour4_mov1 = 0x00C3C6EC;
    float jdcDelay = 0.0f;
uintptr_t DanteJdc::jmp_ret5{NULL};
    float jdcOffset = 0.0f;
    float jdcOffsetAdd = 4.0f;
    int jdcHitcountDefault = 5;
    int jdcHitcount        = 5;
uintptr_t DanteJdc::jmp_ret6{NULL};
    constexpr uintptr_t detour6_call1 = 0x0050CB80;
uintptr_t DanteJdc::jmp_ret7{NULL};
uintptr_t DanteJdc::jmp_ret8{NULL};
uintptr_t DanteJdc::jmp_ret9{NULL};
    float jdcSDprojectileDelay = 40.0f;
    constexpr uintptr_t detour9_comiss1 = 0x00E163C8;
uintptr_t DanteJdc::jmp_ret10{NULL};
    float jdcBounce = 7.0f;
    float jdcEpsilon = 0.001f;
    constexpr uintptr_t detour10_call1 = 0x007BA210;
uintptr_t DanteJdc::jmp_ret11{NULL};
uintptr_t DanteJdc::jmp_ret12{NULL};
uintptr_t DanteJdc::jmp_ret13{NULL};
    constexpr uintptr_t detour13_jmp = 0x007E191A;
    constexpr uintptr_t detour13_call1 = 0x00821450;
    constexpr uintptr_t detour13_call2 = 0x008213B0;
    constexpr uintptr_t detour13_vfx_call = 0x480480;
    constexpr uintptr_t fld_addr = 0x00B9A238;
    float sheatheFrame = 91.0f;

void DanteJdc::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3E15D1, patch1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E18A7, patch1, "\xBA\x92\x00\x00\x00", 5);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

// set jdc vars
naked void detour1(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            pushad
            mov ebp, [static_mediator_ptr] // 0x00E558B8
            mov ebp, [ebp]
            mov ebp, [ebp+0x24]
            mov eax, [ebp+0x16D0]
            cmp byte ptr [DanteJdc::alt_input_enabled], 0
            je flagset
            mov eax, [ebp+0x2260]
        flagset:
            test eax, eax
            je handler
            mov byte ptr [jdcFlag], 1
            mov eax, [ebp+0x3080]
            test eax,eax
            je handler
            fld [eax+0x40]
            fstp [jdcXCoord]
            fld [eax+0x44]
            fstp [jdcYCoord]
            fld [eax+0x48]
            fstp [jdcZCoord]
            push detour1_push1
            mov ecx, [ebp+0x00001D64]
            mov edx, [detour1_mov1] // 00E559D8
            mov edx, [edx] // added this so we don't mov our addr
            push 01
            push 01
            lea eax, [jdcXCoord]
            push eax
            push ebp
            push 0x000000D3 // sound id
            push ecx
            push edx
            call detour1_call1 // spawn sound // 00A47AA0
            popad
            jmp originalcode

        handler:
            mov byte ptr [jdcFlag], 0
            popad
		originalcode:
            mov ecx,esi
            fstp dword ptr [esp+0x04]
        // jmp_ret:
			jmp dword ptr [DanteJdc::jmp_ret1]
    }
}

// spawn jdc
naked void detour2(void) {
    _asm {
        pushfd
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode
        
            pushad
            mov ebp, [static_mediator_ptr]
            mov ebp, [ebp]
            mov ebp, [ebp+0x24]
            mov al, [jdcFlag]
            test eax, eax
            je handler
            mov eax, [ebp+0x000028A4]
            movss xmm0, [eax+0x18]
            comiss xmm0, [jdcTimer1]
            jb JdcGate
            //movss xmm1,[jdcTimer2]
            //addss xmm1,[ebp+0x10]
            //comiss xmm0,xmm1
            //jnb handler
            cmp byte ptr [canJdc],1
            jne handler
            mov eax, [ebp+0x3080]
            call detour2_call1 // Spawn effect + hitbox // DevilMayCry4_DX9.exe+41F970
            mov byte ptr [canJdc],0
            popad
            jmp originalcode

        JdcGate:
            mov byte ptr [canJdc],1
        handler:
            popad
		originalcode:
            popfd
            sub eax, 01
            jne jmp_jne
        //jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret2]
        jmp_jne:
            jmp dword ptr [detour2_jne] // DevilMayCry4_DX9.exe+3E134D
    }
}

// Tracking slash dimension
naked void detour3(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push eax
            push ebp
            mov ebp, [static_mediator_ptr]
            mov ebp, [ebp]
            mov ebp, [ebp+0x24]
            cmp dword ptr [ebp+0x00001564], 0x6B
            je handler
            cmp dword ptr [ebp+0x00001564], 0x6C
            je handler
            mov eax, [ebp+0x3080]
            movss xmm0, [jdcXCoord]
            movss xmm1, [jdcYCoord]
            movss xmm2, [jdcZCoord]
            movss [esi+0x30], xmm0
            movss [esi+0x34], xmm1
            movss [esi+0x38], xmm2
        code:
            pop ebp
            pop eax
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret3]

        originalcode:
            fld dword ptr [edi+0x08]
            fstp dword ptr [esi+0x38]
            jmp jmp_return

        handler:
            fld dword ptr [edi+0x08]
            fstp dword ptr [esi+0x38]
            jmp code
    }
}

// Reduce JDC hitbox delay
naked void detour4(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            movss xmm0, [jdcDelay]
            jmp jmp_return

		originalcode:
            push eax // movss xmm0, [detour_mov1] movs our addr rather than the value
            mov eax, [detour4_mov1]
            movss xmm0, [eax] // DevilMayCry4_DX9.exe+83C6EC
            pop eax
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret4]
    }
}

// Multi-hit jdc
naked void detour5(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push ebp
            push eax
            mov ebp, esi
            movss xmm0, [ebp+0x10]
            addss xmm0, [ebp+0x1790]
            comiss xmm0, [jdcOffset]
            jb handler
            mov eax, [jdcHitcount]
            cmp eax, 1
            dec eax
            mov [jdcHitcount], eax
            je handler2
            movss xmm1, [jdcOffset]
            addss xmm1, [jdcOffsetAdd]
            movss [jdcOffset], xmm1
            mov byte ptr [esi+0x15], 00
            jmp handler

        handler2:
            xorps xmm1,xmm1
            movss [jdcOffset], xmm1
        handler:
            pop eax
            pop ebp
		originalcode:
            cmp byte ptr [esi+0x15], 00
            movss xmm0, [esi+0x10]
        // jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret5]
    }
}

// Stop jdc despawn timer
naked void detour6(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push eax
            mov eax, [jdcHitcount]
            test eax,eax
            je handler
            mov byte ptr [esi+0x14], 00
            pop eax
            jmp code

        handler:
            push ecx
            mov ecx, [jdcHitcountDefault]
            mov [jdcHitcount], ecx
            pop ecx
            mov byte ptr [esi+0x14], 02
            pop eax
        code:
            call detour6_call1
            jmp jmp_return

		originalcode:
            mov byte ptr [esi+0x14], 01
            call detour6_call1
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret6]
    }
}

// Change SD effect
naked void detour7(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            mov edx, 0x000000A2
            jmp jmp_return

		originalcode:
            mov edx, 0x0000008F
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret7]
    }
}

// Change SD anim
naked void detour8(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push 0x00000905
            jmp jmp_return

		originalcode:
            push 0x00000900
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret8]
    }
}

// Reduce SD projectile delay
naked void detour9(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            comiss xmm0, [jdcSDprojectileDelay]
            jmp jmp_return
		originalcode:
            push eax
            mov eax, [detour9_comiss1]
            comiss xmm0, [eax] // [DevilMayCry4_DX9.exe+A163C8]
            pop eax
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret9]
    }
}

// JDC inertia carry
naked void detour10(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode
            cmp byte ptr [DanteJdc::inertia_enabled], 0
            je originalcode

            push eax
            mov al, [jdcFlag]
            test al, al
            je handler
            movss xmm6, [esi+0x1E1C]
            comiss xmm6, [jdcEpsilon]
            jb handler
            mov eax,esi
            call detour10_call1
            pop eax
            fld [esi+0x1E30]
            fstp [esi+0x00000EC0]
            fld [esi+0x1E38]
            fstp [esi+0xEC8]
            fld [jdcBounce]
            fstp [esi+0xEC4]
            jmp jmp_return

        handler:
            pop eax
		originalcode:
            movss [esi+0x00000EC4], xmm0
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret10]
    }
}

// JDC inertia enable
naked void detour11(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode
            cmp byte ptr [DanteJdc::inertia_enabled], 0
            je originalcode

            push eax
            mov al, [jdcFlag]
            test al,al
            je handler
            pop eax
            jmp jmp_return
        handler:
            pop eax
		originalcode:
            movss [esi+0x00001E1C], xmm0
            movss [esi+0x00001E20], xmm0
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret11]
    }
}

// Reduce screenshake
naked void detour12(void) {
    _asm {
            mov eax, [edx+0x000000D0] // org code
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            cmp dword ptr [jdcHitcount], 1
            je originalcode

            xor eax, eax

        originalcode:
            jmp [DanteJdc::jmp_ret12]
    }
}

naked void detour13() {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je code

            cmp byte ptr [esi+0x00002A54], 01
            je retcode

            cmp word ptr [esi+0x334],0x0908
            je conditional_jump

            movss xmm0, [esi+0x348]
            comiss xmm0, [sheatheFrame]
            jb conditional_jump

            fld1
            sub esp,0x0C
            fstp dword ptr [esp+0x08]
            mov al,0x01
            fldz
            mov ecx,esi
            fstp dword ptr [esp+0x04]
            fld dword ptr [fld_addr]
            fstp dword ptr [esp]
            push 0x00
            push 0x00000908
            call [detour13_call1]
            push esi
            push edi
            mov esi,0x00000004
            xor edi,edi
            call [detour13_call2]
            pop edi
            pop esi
            push esi
            push 0x14
            push -0x01
            push 0x08
            push esi
            mov edx,0xA2
            mov eax,2
            call [detour13_vfx_call]
            jmp [detour13_jmp]

        code:
            cmp byte ptr [esi+0x00002A54],01
        retcode:
            jmp [DanteJdc::jmp_ret13] // jumps to jne detour13_jmp
        conditional_jump:
            jmp [detour13_jmp]
    }
}

std::optional<std::string> DanteJdc::on_initialize() {
    if (!install_hook_offset(0x3E1189, hook1, &detour1, &jmp_ret1, 6)) { // set jdc vars
        spdlog::error("Failed to init DanteJdc mod\n");
        return "Failed to init DanteJdc mod";
    }
    if (!install_hook_offset(0x3E110C, hook2, &detour2, &jmp_ret2, 9)) { // spawn jdc
        spdlog::error("Failed to init DanteJdc mod2\n");
        return "Failed to init DanteJdc mod2";
    }
    if (!install_hook_offset(0x41F9C2, hook3, &detour3, &jmp_ret3, 6)) { // Tracking slash dimension
        spdlog::error("Failed to init DanteJdc mod3\n");
        return "Failed to init DanteJdc mod3";
    }
    if (!install_hook_offset(0x41F78B, hook4, &detour4, &jmp_ret4, 8)) { // Reduce JDC hitbox delay
        spdlog::error("Failed to init DanteJdc mod4\n");
        return "Failed to init DanteJdc mod4";
    }
    if (!install_hook_offset(0x41F8A3, hook5, &detour5, &jmp_ret5, 9)) { // Multi-hit jdc
        spdlog::error("Failed to init DanteJdc mod5\n");
        return "Failed to init DanteJdc mod5";
    }
    if (!install_hook_offset(0x41F95D, hook6, &detour6, &jmp_ret6, 9)) { // Stop jdc despawn timer
        spdlog::error("Failed to init DanteJdc mod6\n");
        return "Failed to init DanteJdc mod6";
    }
    if (!install_hook_offset(0x3E183B, hook7, &detour7, &jmp_ret7, 5)) { // Change SD effect
        spdlog::error("Failed to init DanteJdc mod7\n");
        return "Failed to init DanteJdc mod7";
    }
    if (!install_hook_offset(0x3E17C3, hook8, &detour8, &jmp_ret8, 5)) { // Change SD anim
        spdlog::error("Failed to init DanteJdc mod8\n");
        return "Failed to init DanteJdc mod8";
    }
    if (!install_hook_offset(0x3E15F9, hook9, &detour9, &jmp_ret9, 7)) { // Reduce SD projectile delay
        spdlog::error("Failed to init DanteJdc mod9\n");
        return "Failed to init DanteJdc mod9";
    }
    if (!install_hook_offset(0x3E12FE, hook10, &detour10, &jmp_ret10, 8)) { // JDC inertia carry
        spdlog::error("Failed to init DanteJdc mod10\n");
        return "Failed to init DanteJdc mod10";
    }
    if (!install_hook_offset(0x3E12AD, hook11, &detour11, &jmp_ret11, 16)) { // JDC inertia enable
        spdlog::error("Failed to init DanteJdc mod11\n");
        return "Failed to init DanteJdc mod11";
    }
    if (!install_hook_offset(0x41F932, hook12, &detour12, &jmp_ret12, 6)) { // Reduce screenshake
        spdlog::error("Failed to init DanteJdc mod12\n");
        return "Failed to init DanteJdc mod12";
    }
    if (!install_hook_offset(0x3E15CA, hook13, &detour13, &jmp_ret13, 7)) {
        spdlog::error("Failed to init DanteJdc mod13\n");
        return "Failed to init DanteJdc mod13";
    }
    return Mod::on_initialize();
}

static bool fileExists = false;

/*static void SeeIfFileExists() {
    FILE* file = nullptr;
    errno_t errorCheck = fopen_s(&file, ".\\nativePC\\Collision\\wp002.col", "r");
    if (errorCheck == 0 && file != nullptr) {
        fileExists = true;
        fclose(file);
    } else {
        fileExists = false;
        DanteJdc::mod_enabled = false;
    }
}*/

#include <filesystem>
static void SeeIfFileExists() {
    fileExists = std::filesystem::exists(".\\nativePC\\Collision\\wp002.col");
    if (!fileExists) {
        DanteJdc::mod_enabled = false;
    }
}

void DanteJdc::on_gui_frame() {
    ImGui::BeginGroup();
    if (!fileExists) {
        if (ImGui::Button(_("Download Judgement Cut Files"))) {
            ShellExecuteA(NULL, "open", "https://github.com/muhopensores/dmc4_hook/releases", NULL, NULL, SW_SHOWNORMAL);
        }
        ImGui::SameLine();
        help_marker(_("Clicking this button will open https://github.com/muhopensores/dmc4_hook/releases\n"
            "From here you can download optional files for mods that require them, found in the Assets section of each dmc4_hook release\n"
            "Once you've downloadeded and installed these files (I recommend using Fluffy Mod Manager), "
            "restart the game and tick \"HDD File Priority\" in the Debug tab before you load into a stage\n"
            "This mod gives Dante Judgement Cut"));
    }
    else {
        if (ImGui::Checkbox(_("Judgement Cut"), &mod_enabled))
            toggle(mod_enabled);
        ImGui::SameLine();
        help_marker(_("Activate judgement cut when performing Yamato aerial rave with lock-on."
            "Perform normal inertia-less Yamato rave on lock-off\n"
            "Requires \"HDD File Priority\" at the top of the Debug page"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
                ImGui::Checkbox(_("Lock-on + back input"), &alt_input_enabled);
                ImGui::SameLine();
                help_marker(_("Bind jdc activation to lock-on + back"));
                ImGui::Checkbox(_("Inertia enable"), &DanteJdc::inertia_enabled);
            ImGui::Unindent(lineIndent);
        }
    }
    ImGui::EndGroup();
}

void DanteJdc::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dante_jdc").value_or(false);
    SeeIfFileExists();
    if (mod_enabled) toggle(mod_enabled);
    alt_input_enabled = cfg.get<bool>("jdc_alt_input").value_or(false);
    DanteJdc::inertia_enabled = cfg.get<bool>("jdc_inertia").value_or(true);
}

void DanteJdc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dante_jdc", mod_enabled);
    cfg.set<bool>("jdc_alt_input", alt_input_enabled);
    cfg.set<bool>("jdc_inertia", DanteJdc::inertia_enabled);
}
#endif
