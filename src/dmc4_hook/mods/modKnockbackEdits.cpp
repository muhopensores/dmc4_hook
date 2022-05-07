#include "modKnockbackEdits.hpp"
#if 1

bool KnockbackEdits::modEnabled{ false };
uintptr_t KnockbackEdits::jmp_return{ NULL };

static bool releaseStuns{ false };

/*
class attackDataEntry
{
public:
    char name[12];    // 0x1365888
    __int32 unk00;    // 0x000C
    __int32 unkInt00; // 0x0010
    float   damage;   // 0x0014
    __int32 unkInt01; // 0x0018
    __int32 unkInt02; // 0x001C // level 2 release changes this from 0 to 2 to make it stun the enemy. Looks like this is stun value - can stun DT enemies instantly with a high enough value
    __int32 isGround; // 0x0020 // 00 neither 01 applies displacement when enemy is aerial 02 applies displacement when enemy is either air/ground
    __int32 unkInt04; // 0x0024 // i think 00 == melee, 01 == gun. 01 has different hit vfx and kills ghost blanket quickly
    __int32 unkInt05; // 0x0028 // hitstop duration
    __int32 unkInt06; // 0x002C
    __int32 unkInt07; // 0x0030
    __int32 kbType;   // 0x0034
    __int32 unkInt08; // 0x0038
    __int32 unkInt09; // 0x003C
    Vector3 unkVec3;  // 0x0040
    __int32 unkInt10; // 0x004C
    __int32 unkInt11; // 0x0050 // setting this to 02 breaks knockback direction and always sends the enemy a certain direction. Shotgun uses 1 and has interesting behaviour
    __int32 end;      // 0x0054

}; // Size=0x0058

class atkData
{
public:
    char header[4];           // 0x1365888
    DWORD headerWord;         // 0x0004
    attackDataEntry name[64]; // 0x0008

}; // Size=0x1608

class attStruct
{
public:
    char pad_0x0000[0x6C];  // 0x0000
    atkData* atkDataPtr;    // 0x006C
    char pad_0x0070[0x794]; // 0x0070

}; // Size=0x0804


class kbStruct
{
public:
    virtual create;           //
    virtual void Function1(); //
    virtual void Function2(); //
    virtual void Function3(); //
    virtual void Function4(); //
    virtual void Function5(); //
    virtual void Function6(); //
    virtual void Function7(); //
    virtual void Function8(); //
    virtual void Function9(); //

    char pad_0x0004[0x17EC]; // 0x0004
    attStruct* attPtr;       // 0x17F0
    char pad_0x17F4[0x4C];   // 0x17F4

}; // Size=0x1840
*/

naked void detour() { // projectiles?
    _asm {
        mov esi, edx // edx = string of move at the start of info
        repe movsd // esi incs 4 every repe movsd starting at edx
        pop edi

		//cmp byte ptr [KnockbackEdits::modEnabled], 0
		//je retcode
        cmp byte ptr [releaseStuns], 1
        jne retcode

        cmp [eax+0xA4+0x00], 1162626386 // RELE(ASE) // get any release
        je releaseCheck2
        jmp retcode

    releaseCheck2:
        cmp [eax+0xA4+0x08], 3145793 // A 0 // get level 1 release
        jne retcode
        // cmp byte ptr [releaseStuns], 1
        // jne retcode
        mov word ptr [eax+0xA4+0x1C], 2 // make it hit
        jmp retcode

    retcode:
		jmp dword ptr [KnockbackEdits::jmp_return]
    }
}

std::optional<std::string> KnockbackEdits::onInitialize() {
    if (!install_hook_offset(0x1099F8, hook, &detour, &jmp_return, 5)) { // projectiles?
        HL_LOG_ERR("Failed to init KnockbackEdits mod\n");
        return "Failed to init KnockbackEdits mod";
    }

    return Mod::onInitialize();
}

void KnockbackEdits::onGUIframe() {
    //ImGui::Checkbox("Stun/Knockback Edits", &modEnabled);
    ImGui::Checkbox("Release Always Stuns", &releaseStuns);
}

void KnockbackEdits::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("knockback_edits").value_or(false);
    releaseStuns = cfg.get<bool>("release_always_stuns").value_or(false);
}

void KnockbackEdits::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("knockback_edits", modEnabled);
    cfg.set<bool>("release_always_stuns", releaseStuns);
}

#endif
