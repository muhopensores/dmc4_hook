/*
#include "../mods.h"
#include "modEnemySpawn.hpp"

#if 1

bool EnemySpawn::modEnabled{ false };
// uintptr_t EnemySpawn::_enemySpawnContinue{ NULL };

EnemySpawn::EnemySpawn()
{
    // onInitialize();
}

naked void enemySpawn_proc(void)
{
    _asm {
		cmp byte ptr [EnemySpawn::modEnabled], 0
		je code
        ret // ?

    code:
        call 0055E710h //Scarecrow Arm
        //does jackshit
        //pushad
        //fld dword ptr [float1]
        //fld dword ptr [float2]
        //fld dword ptr [float3]
        //popad
        //pushad
        //lea eax, [eax+1898]
        //fstp dword ptr [eax]
        //lea eax, [eax-04]
        //fstp dword ptr [eax]
        //lea eax, [eax-04]
        //fstp dword ptr [eax]
        //popad
        mov esi, eax
        mov ecx, 0Fh
        //mov ecx, 126
        mov eax, [00E552CCh]
        push 0Fh
        call 008DC540h
        ret
    }
}


//arm crow call 0055E710
//leg crow call 0053F810
//frost call 005A3F60
//white armours call 00561A10
//ghosties call 0057F1E0


std::optional<std::string> EnemySpawn::onInitialize()
{
    if (!install_hook_offset(0x0, hook, &enemySpawn_proc, &EnemySpawn::_enemySpawnContinue, 5))
    {
        HL_LOG_ERR("Failed to init EnemySpawn mod\n");
        return "Failed to init EnemySpawn mod";
    }

    return Mod::onInitialize();
}

    void EnemySpawn::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    ImGui::Checkbox("Enemy Spawn", &modEnabled);
}
/*
void EnemySpawn::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("enemy_spawn").value_or(false);
};

void EnemySpawn::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("enemy_spawn", modEnabled);
};

#endif

*/