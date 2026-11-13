#pragma once

#include <cstdint>

// Enemy step (jump cancel) targets beyond the game's enemies.
//
// uPlayer::checkEmJumpBegin 0x804970 (edi = the jumper, any uPlayer: players and the boss Dante) passes its gates (the
// save's enemy-step skill picked by mPlayerID, +0x2005, the action's step bit mActStat +0x1554 & 2, a jump press, the
// step timer +0x1FF4 <= 0), then looks for a target on move lines 15 and 16 with uActor::isEmJump 0x4AB170. That
// measures the target's step points against sMediator::mpPlayer + 85 cm, not against the jumper, and players (line
// 13) are never looked at. The hook sits at 0x804A37, between the two line scans (every early return shares 0x804A71,
// so it can't go later): each registered source is asked for a target for this jumper, and the first one given goes on
// at 0x804A7C as the game's own hit does (the target's enemyJumpMessage, vtable +0xE8, a nullsub on players; the step
// timers; action 25).
//
// A mod installs the hook once and registers its sources:
//     if (!enemy_step::hook_installed) { install_hook_absolute(enemy_step::kSite, hook, &enemy_step::detour, &enemy_step::jmp_ret, 5); enemy_step::hook_installed = true; }
//     enemy_step::add_target_source(&my_source);
namespace enemy_step {

constexpr uintptr_t kSite = 0x804A37;   // bytes replaced: `mov eax, sUnit::mpInstance` (5); jmp_ret = 0x804A3C

// A target for this jumper, or 0. Runs in the jumper's tick (the jumper's move line), so it must only read.
using TargetFn = uintptr_t(*)(uintptr_t jumper);
bool add_target_source(TargetFn fn);

// The step check measured from the jumper: the target is a live, moving actor (as isEmJump wants: not +0xEA8 & 0x80,
// unit state 2 with 0x400) and the world position of its joint `joint` (+0x2E8 joint table, +0x2E4 joints of 0x90
// bytes) is within `radius` cm of the jumper's position + 85 cm up.
constexpr int   kChestJoint  = 2;
constexpr float kPlayerReach = 300.0f;   // a player as a step target: his chest, the radius the Coop rule has used
bool in_reach(uintptr_t jumper, uintptr_t target, int joint = kChestJoint, float radius = kPlayerReach);

extern bool hook_installed;
extern uintptr_t jmp_ret;
void detour();

} // namespace enemy_step
