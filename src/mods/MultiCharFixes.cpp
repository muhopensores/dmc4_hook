#include "MultiCharFixes.hpp"
#include "..\\sdk\\uActor.hpp"

bool MultiCharFixes::targeting_fix = false;
bool MultiCharFixes::jc_fix = false;
uintptr_t MultiCharFixes::jmp_ret1 = NULL;
uintptr_t MultiCharFixes::jmp_ret2 = NULL;
static uintptr_t isEmJump_call     = 0x4ab170;

void MultiCharFixes::toggle_targeting_fix(bool enable) {
    if (enable) {
        install_patch_offset(0x420880, patch1, "\x90\x90\x90", 3);
        install_patch_offset(0x420891, patch2, "\x90\x90\x90", 3);
    } else {
        patch1.reset();
        patch2.reset();
    }
}

bool __stdcall new_jc_check(uActor* pl, uActor* em) {
    if ((em->mEmJumpEnableFlag != false) && (em->mpEmJumpData != nullptr) && ((em->mActorStatus & 0x80) == 0) &&
        (em->flags.bits.mBeFlag == 2) && ((em->flags.raw & 0x400) != 0)) {
        
        uint32_t jc_mask = ((uint32_t(__stdcall *)(uActor * actor))(0x04ab3a0))(em);
        if (jc_mask == 0) return false;
        
        kEmJumpData* jc_data = em->mpEmJumpData;
        if (jc_data->jointNo == -1) return false;
        
        int count = 0;
        
        do {
            if ((jc_mask & (1 << (count & 0x1f))) != 0) {
                uint32_t joint_no = jc_data->jointNo;
                joint_no = em->mJointTable[joint_no & 0xff];
                
                UModelJoint* pjoint;
                if (joint_no == 0xff) {
                    count++;
                    jc_data++;
                    continue;
                }
                pjoint = &em->mpJoint[joint_no];
                
                MtMatrix* joint_mat = &pjoint->mWmat;
                
                MtVector4 pl_pos;
                if (pl != nullptr) {
                    pl_pos.x = pl->mPos.x;
                    pl_pos.y = pl->mPos.y + 85.0f;
                    pl_pos.z = pl->mPos.z;
                    pl_pos.w = 1.0f;
                } else {
                    pl_pos.x = 0.0f;
                    pl_pos.y = 0.0f;
                    pl_pos.z = 0.0f;
                    pl_pos.w = 1.0f;
                }
                
                MtVector4 ofs = {jc_data->offset.x, jc_data->offset.y, jc_data->offset.z, 1.0f};
                glm::vec4 transformed_pos = glm::make_mat4((float*)joint_mat) * glm::make_vec4((float*)&ofs);
                
                glm::vec3 diff = glm::vec3(transformed_pos) - glm::vec3(pl_pos.x, pl_pos.y, pl_pos.z);
                float dist_squared = glm::dot(diff, diff);
                
                if (dist_squared < jc_data->radius * jc_data->radius) {
                    return true;
                }
            }
            
            count++;
            jc_data++;
            
        } while (jc_data->jointNo != -1);
    }
    return false;
}

naked void detour1() {
    _asm {
            cmp byte ptr [MultiCharFixes::jc_fix], 1
            jne originalcode

            push edi
            call new_jc_check

            jmp handle
        originalcode:
            call isEmJump_call // player in edi, enemy in esi
        handle:
            jmp [MultiCharFixes::jmp_ret1]
    }
}

naked void detour2() {
    _asm {
            cmp byte ptr [MultiCharFixes::jc_fix], 1
            jne originalcode

            push edi
            call new_jc_check

            jmp handle
        originalcode:
            call isEmJump_call
        handle:
            jmp [MultiCharFixes::jmp_ret2]
    }
}


void MultiCharFixes::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Targeting Fix"), &targeting_fix))
        toggle_targeting_fix(targeting_fix);
    ImGui::SameLine();
    help_marker(_("Prevents characters from drifting with multiple player characters available"));

    ImGui::Checkbox(_("JC Fix"), &jc_fix);
    ImGui::SameLine();
    help_marker(_("Fixes co-op JC"));
}

std::optional<std::string> MultiCharFixes::on_initialize() {
    if (!install_hook_absolute(0x0804a21, hook1, &detour1, &jmp_ret1, 5)) { // Nero HUD
        spdlog::error("Failed to init Coop mod1\n");
        return "Failed to init Coop mod1";
    }
    
    if (!install_hook_absolute(0x804a61, hook2, &detour2, &jmp_ret2, 5)) { // Nero HUD
        spdlog::error("Failed to init Coop mod1\n");
        return "Failed to init Coop mod1";
    }
    return Mod::on_initialize();
}

void MultiCharFixes::on_config_load(const utility::Config& cfg) {
    targeting_fix = cfg.get<bool>("targeting_fix").value_or(false);
    jc_fix = cfg.get<bool>("jc_fix").value_or(false);
    if (targeting_fix) toggle_targeting_fix(targeting_fix);

}

void MultiCharFixes::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("targeting_fix", targeting_fix);
    cfg.set<bool>("jc_fix", jc_fix);
}
