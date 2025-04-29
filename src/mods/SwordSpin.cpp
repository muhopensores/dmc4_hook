#include "SwordSpin.hpp"

bool SwordSpin::mod_enabled {false};

uintptr_t SwordSpin::jmp_ret1 = NULL;
    constexpr uintptr_t detour1_jmp = 0x839E20;
    constexpr uintptr_t collision_call = 0x0050CA60;
    Matrix4x4 anchorMat;
    float hitFloat1 = 5.0f;
    float hitFloat2 = 0.0f;
    float rotAngle = 0.0f;
    float rotSpeed = 0.15f;
    float radiusAdd = 0.0f;
uintptr_t SwordSpin::jmp_ret2 = NULL;

void __stdcall mat_rot(float* target_pos, float* pos_addr, float tick) {
    anchorMat[3].x = target_pos[0];
    anchorMat[3].y = target_pos[1];
    anchorMat[3].z = target_pos[2];

    glm::mat4 RotateY   = glm::rotate(anchorMat, rotAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 Shift2 = glm::translate(RotateY, glm::vec3(0.f, 150.0f, 250.0f+radiusAdd));

    pos_addr[0]         = Shift2[3].x;
    pos_addr[1]         = Shift2[3].y;
    pos_addr[2]         = Shift2[3].z;

    rotAngle += tick*rotSpeed;
    if (radiusAdd < 50.0f)
        radiusAdd += tick * 0.5f;;
}

naked void detour1(void) {
    _asm {
            cmp byte ptr [SwordSpin::mod_enabled],1
            jne originalcode

            pushad
            //blur
            mov eax,[ebx+0x17A0] //uPlSwordBlur
            mov eax,[eax+0x74]
            or word ptr [eax+4],0x0800
            mov byte ptr [eax+0x16],2
            mov byte ptr [eax+0x17],0
            fld1
            fstp [eax+0x254]
            fld1
            fstp [eax+0x258]
            mov eax,[ebx+0x17A0] // uPlSwordBlur
            mov eax,[eax+0x78]
            or word ptr [eax+4],0x0800
            mov byte ptr [eax+0x16],2
            mov byte ptr [eax+0x17],0
            fld1
            fstp [eax+0x254]
            fld1
            fstp [eax+0x258]
            //Collision
            lea eax,[ebx+0x13A0]
            push 00
            push eax
            or edx,-01
            xor eax,eax
            call collision_call
            test eax,eax
            je tick
            fld [hitFloat1]
            fstp [eax+0x14C]
            fld [hitFloat2]
            fstp [eax+0x148]
            //mov byte ptr [eax+0x64],1
        tick:
            push eax
            fld [ebx+0x10]
            fstp [esp]
            //pos_addr
            lea eax,[ebx+0x30]
            push eax
            //WMat
            mov eax,[ebx+0x1378]
            lea eax,[eax+0x30]
            push eax
            call mat_rot
            popad

            lea ecx,[ebx+0x13A0]
            jmp detour1_jmp
        originalcode:
            movss xmm1,[ebx+0x10]
            jmp [SwordSpin::jmp_ret1]
    }
}

void __stdcall set_spin_vars(float* srcMat) {
    anchorMat = glm::make_mat4x4(srcMat);
    rotAngle = 0.0f;
    radiusAdd = 0.0f;
}

naked void detour2(void) {
    _asm {
            cmp byte ptr [SwordSpin::mod_enabled],1
            jne originalcode

            pushad
            lea eax,[edi+0xA0]
            push eax
            call set_spin_vars
            popad
        originalcode:
            or word ptr [edi+0x000028BE],04
            jmp [SwordSpin::jmp_ret2]
    }
}

std::optional<std::string> SwordSpin::on_initialize() {
     if (!install_hook_offset(0x439DCC, hook1, &detour1, &jmp_ret1, 5)) {
	 	spdlog::error("Failed to init SwordSpin mod\n");
	 	return "Failed to init SwordSpin mod";
	 }
     if (!install_hook_offset(0x3CF2B8, hook2, &detour2, &jmp_ret2, 8)) {
         spdlog::error("Failed to init SwordSpin mod2\n");
         return "Failed to init SwordSpin mod2";
     }
    return Mod::on_initialize();
}

void SwordSpin::on_gui_frame(int display) {
    ImGui::Checkbox(_("Sword Spin"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Round trip orbits the player when not locked onto a target"));
}

void SwordSpin::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("sword_spin").value_or(false);
};

void SwordSpin::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("sword_spin", mod_enabled);
};