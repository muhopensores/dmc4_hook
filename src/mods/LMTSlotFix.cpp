#include "LMTSlotFix.hpp"
#include "MoveTable.hpp"
#include "AerialStinger.hpp"
#include "Payline.hpp"
#include "LuciAirThrow.hpp"
#include "..\sdk\cResource.hpp"
#include "..\sdk\uActor.hpp"
// bool LMTSlotFix::mod_enabled = false;

uintptr_t LMTSlotFix::jmp_ret1 = NULL;
constexpr uintptr_t detour1_default_jmp = 0x42B945;
constexpr uintptr_t detour1_jmp         = 0x42B945;

/*void LMTSlotFix::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x2B934, patch1, "\x90\x90", 2);//break infinite loop referencing cCharTblMgr
    }
    else {
        patch1.reset();
    }
}*/

struct rCharTbl : cResource {
    uint8_t* mpRsrcTop; // 0x60 the file as loaded
    uint8_t* mpDataTop; // 0x64 mpRsrcTop + 4
};
static_assert(sizeof(rCharTbl) == 0x68);

struct kContactData {
    uint8_t info[6];
};

struct cCharTblMgr : MtObject {
    rCharTbl* mpRsrc;              // 0x04
    Motion* mpMtn;                 // 0x08 the motion this table follows
    uint8_t* mpDataTop;            // 0x0C start of the current script
    uint8_t* mpDataCrnt;           // 0x10 next command
    bool mIsDone;                  // 0x14
    kContactData mContactData[2];  // 0x15
    cCharTblMgr* set();
};
static_assert(sizeof(cCharTblMgr) == 0x24);
static_assert(offsetof(cCharTblMgr, mIsDone) == 0x14);
static_assert(offsetof(cCharTblMgr, mContactData) == 0x15);

static uint32_t read_u32(const uint8_t* p) {
    return *reinterpret_cast<const uint32_t*>(p);
}

static uint8_t* find_script(const rCharTbl* table, uint32_t bank, uint32_t slot) {
    uint8_t* bank_table = table->mpDataTop + read_u32(table->mpDataTop + 4 * bank);
    return bank_table + read_u32(bank_table + 4 * slot);
}

cCharTblMgr* cCharTblMgr::set() {
    const uint32_t motion_no = mpMtn->mMotionNo;
    if (motion_no == 0xFFFF) { // no motion: no script
        mpDataTop  = nullptr;
        mpDataCrnt = nullptr;
        mIsDone    = true;
        return this;
    }
    memset(mContactData, 0, sizeof(mContactData));
    const uint32_t bank = (motion_no >> 8) & 0xF; // the LMT file: pl000_NN = bank NN - 1
    const uint32_t slot = motion_no & 0xFF;       // the block in that LMT
    mpDataTop  = find_script(mpRsrc, bank, slot);
    mpDataCrnt = mpDataTop;
    mIsDone    = false;
    return this;
}


naked void detour1() {
    _asm {
            // cmp byte ptr [Payline::mod_enabled], 1
            // je newcode
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            cmp eax,3
            ja detour1_ja
        originalcode:
            cmp eax,3
            ja detour1_default_ja
            jmp [LMTSlotFix::jmp_ret1]
        detour1_ja:
            jmp detour1_jmp
        detour1_default_ja:
            jmp detour1_default_jmp
    }
}

std::optional<std::string> LMTSlotFix::on_initialize() {
    if (!install_hook_offset(0x2B8FF, hook1, &detour1, &jmp_ret1, 5))
    {
        spdlog::error("Failed to init LMTSlotFix mod1\n");
        return "Failed to init LMTSlotFix mod1";
    }
    return Mod::on_initialize();
}

/*void LMTSlotFix::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("LMT slot fix"), &mod_enabled)) {
            //toggle(mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("Enable reusing zeroed out offsets in LMT files. DO NOT disable if you have custom motion mods."));
    }
}

void LMTSlotFix::on_config_load(const utility::Config& cfg) {
    // mod_enabled = cfg.get<bool>("lmt_slot_fix").value_or(false);
    // toggle(mod_enabled);
};

void LMTSlotFix::on_config_save(utility::Config& cfg) {
    // cfg.set<bool>("lmt_slot_fix", mod_enabled);
};*/
