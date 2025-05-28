#pragma once

#include "../mod.hpp"

class KnockbackEdits : public Mod {
public:
    KnockbackEdits() = default;

    static bool mod_enabled;
    static uintptr_t jmp_return0;
	static uintptr_t jmp_return1;
    static uintptr_t jmp_return2;

    std::string get_mod_name() override { return "KnockbackEdits"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook0, hook1, hook2;
};

enum ATTACK_ID /* int */
{
	ATTACK_ID_DEFAULT = 0x0,
	ATTACK_ID_NERO_STREAK = 0x1,
	ATTACK_ID_NERO_HIGHROLLER = 0x2,
	ATTACK_ID_DANTE_STINGER = 0x3,
	ATTACK_ID_DANTE_HIGHTIME = 0x4,
	ATTACK_ID_NERO_GRAB_LONG = 0x5,
	ATTACK_ID_NERO_GRAB_SHORT = 0x6,
	ATTACK_ID_NERO_CHARGE_SHOT_LV1 = 0x7,
	ATTACK_ID_NERO_CHARGE_SHOT_LV2 = 0x8,
	ATTACK_ID_NERO_CHARGE_SHOT_LV3 = 0x9,
	ATTACK_ID_WIND0 = 0xa,
	ATTACK_ID_WIND1 = 0xb,
	ATTACK_ID_WIND2 = 0xc,
	ATTACK_ID_HIT_CHECK = 0xd,
	ATTACK_ID_EM012_ELECTRIGGER = 0xe,
	ATTACK_ID_EM005_CHANGE = 0xf,
	ATTACK_ID_NERO_CHARGE_SHOT_LV3_BOMB = 0x10,
	ATTACK_ID_SHORT_SWING_SLASH = 0x11,
	ATTACK_ID_MILLION_STAB = 0x12,
	ATTACK_ID_EM022_SPEAR_BUSTER_PL = 0x13,
	ATTACK_ID_EM023_ABSORB = 0x14,
	ATTACK_ID_GILGAMESH_LV1 = 0x15,
	ATTACK_ID_GILGAMESH_LV2 = 0x16,
	ATTACK_ID_GILGAMESH_LV3 = 0x17,
	ATTACK_ID_FIRE_DAMAGE = 0x18,
	ATTACK_ID_EM019_EAT = 0x19,
	ATTACK_ID_HUGE_STATUE_ATTACK = 0x1a,
	ATTACK_ID_EM023_ABSORB_HIGH = 0x1b,
	ATTACK_ID_EM019_HYOUKETSU_TSUKAMI = 0x1c,
	ATTACK_ID_EM021_HEAD = 0x1d,
	ATTACK_ID_NERO_MAJIN_RANBU_FINISH = 0x1e,
	ATTACK_ID_NERO_MAJIN_DRIVE = 0x1f,
	ATTACK_ID_EM023_CALL_DIE = 0x20,
	ATTACK_ID_NERO_EM016_BUSTER = 0x21,
	ATTACK_ID_NERO_EM017_BUSTER = 0x22,
	ATTACK_ID_EM015_BUSTER_END = 0x23,
	ATTACK_ID_EM015_BUSTER_MAKIKOMI = 0x24,
	ATTACK_ID_EM015_BUSTER_MAKIKOMI_MAJIN = 0x25,
	ATTACK_ID_EM025_STRAIGHT = 0x26,
	ATTACK_ID_EM025_UPPER = 0x27,
	ATTACK_ID_EM025_STRAIGHT_LEFT = 0x28,
	ATTACK_ID_EM025_FINAL_FLASH = 0x29,
	ATTACK_ID_EM025_HAMMER_BOTH = 0x2a,
	ATTACK_ID_EM014_ENEMY_BLOCK_ATTACK = 0x2b,
	ATTACK_ID_DANTE_COYOTE = 0x2c,
	ATTACK_ID_EM025_HAMMER_LEFT = 0x2d,
	ATTACK_ID_EM025_STAGE_LASER = 0x2e,
	ATTACK_ID_EM025_STAGE_CANNON = 0x2f,
	ATTACK_ID_TEMPLE_BLADE = 0x30,
	ATTACK_ID_ECHIDNA_B_ATK = 0x31,
	ATTACK_ID_EM025_THROW = 0x32,
	ATTACK_ID_EM025_FINAL_FLASH_FINISH = 0x33,
	ATTACK_ID_EM025_DEATH_BALL = 0x34,
	ATTACK_ID_EM025_DEATH_BALL_EXPLOSION = 0x35,
	ATTACK_ID_EM025_DEATH_BALL2 = 0x36,
	ATTACK_ID_EM025_DEATH_BALL2_EXPLOSION = 0x37,
	ATTACK_ID_EM025_CANNON = 0x38,
	ATTACK_ID_EM025_CANNON_EXPLOSION = 0x39,
	ATTACK_ID_EM025_STAGE_EXPLOSION = 0x3a,
	ATTACK_ID_EM031_ROOM_ATK_FLOOR = 0x3b,
	ATTACK_ID_SNATCH_VERGIL = 0x3c,
	ATTACK_ID_SNATCH_TRISH = 0x3d,
	ATTACK_ID_SNATCH_TRISH_AIR = 0x3e,
	ATTACK_ID_SNATCH_LADY = 0x3f,
	ATTACK_ID_LADY_CHARGE_SHOT_LV1 = 0x40,
	ATTACK_ID_LADY_CHARGE_SHOT_LV2 = 0x41,
	ATTACK_ID_LADY_CHARGE_SHOT_LV3 = 0x42,
	ATTACK_ID_THUNDER_BROW = 0x43,
	ATTACK_ID_COREDER = 0x44,
	ATTACK_ID_LADY_TWOHAND_SHOT_LV1 = 0x45,
	ATTACK_ID_LADY_TWOHAND_SHOT_LV2 = 0x46,
	ATTACK_ID_NUM = 0x47,
};
static const char* attack_id_names[] = {
    "Default",                     // 0x00 ATTACK_ID_DEFAULT
    "Nero Streak",                 // 0x01 ATTACK_ID_NERO_STREAK
    "Nero Highroller",             // 0x02 ATTACK_ID_NERO_HIGHROLLER
    "Dante Stinger",               // 0x03 ATTACK_ID_DANTE_STINGER
    "Dante High Time",             // 0x04 ATTACK_ID_DANTE_HIGHTIME
    "Nero Grab Long",              // 0x05 ATTACK_ID_NERO_GRAB_LONG
    "Nero Grab Short",             // 0x06 ATTACK_ID_NERO_GRAB_SHORT
    "Nero Charge Shot Lv1",        // 0x07 ATTACK_ID_NERO_CHARGE_SHOT_LV1
    "Nero Charge Shot Lv2",        // 0x08 ATTACK_ID_NERO_CHARGE_SHOT_LV2
    "Nero Charge Shot Lv3",        // 0x09 ATTACK_ID_NERO_CHARGE_SHOT_LV3
    "Wind 0",                      // 0x0A ATTACK_ID_WIND0
    "Wind 1",                      // 0x0B ATTACK_ID_WIND1
    "Wind 2",                      // 0x0C ATTACK_ID_WIND2
    "Hit Check",                   // 0x0D ATTACK_ID_HIT_CHECK
    "EM012 Electrigger",           // 0x0E ATTACK_ID_EM012_ELECTRIGGER
    "EM005 Change",                // 0x0F ATTACK_ID_EM005_CHANGE
    "Nero Charge Shot Lv3 Bomb",   // 0x10 ATTACK_ID_NERO_CHARGE_SHOT_LV3_BOMB
    "Short Swing Slash",           // 0x11 ATTACK_ID_SHORT_SWING_SLASH
    "Million Stab",                // 0x12 ATTACK_ID_MILLION_STAB
    "EM022 Spear Buster Player",   // 0x13 ATTACK_ID_EM022_SPEAR_BUSTER_PL
    "EM023 Absorb",                // 0x14 ATTACK_ID_EM023_ABSORB
    "Gilgamesh Lv1",               // 0x15 ATTACK_ID_GILGAMESH_LV1
    "Gilgamesh Lv2",               // 0x16 ATTACK_ID_GILGAMESH_LV2
    "Gilgamesh Lv3",               // 0x17 ATTACK_ID_GILGAMESH_LV3
    "Fire Damage",                 // 0x18 ATTACK_ID_FIRE_DAMAGE
    "EM019 Eat",                   // 0x19 ATTACK_ID_EM019_EAT
    "Huge Statue Attack",          // 0x1A ATTACK_ID_HUGE_STATUE_ATTACK
    "EM023 Absorb High",           // 0x1B ATTACK_ID_EM023_ABSORB_HIGH
    "EM019 Hyouketsu Tsukami",     // 0x1C ATTACK_ID_EM019_HYOUKETSU_TSUKAMI
    "EM021 Head",                  // 0x1D ATTACK_ID_EM021_HEAD
    "Nero Majin Ranbu Finish",     // 0x1E ATTACK_ID_NERO_MAJIN_RANBU_FINISH
    "Nero Majin Drive",            // 0x1F ATTACK_ID_NERO_MAJIN_DRIVE
    "EM023 Call Die",              // 0x20 ATTACK_ID_EM023_CALL_DIE
    "Nero EM016 Buster",           // 0x21 ATTACK_ID_NERO_EM016_BUSTER
    "Nero EM017 Buster",           // 0x22 ATTACK_ID_NERO_EM017_BUSTER
    "EM015 Buster End",            // 0x23 ATTACK_ID_EM015_BUSTER_END
    "EM015 Buster Makikomi",       // 0x24 ATTACK_ID_EM015_BUSTER_MAKIKOMI
    "EM015 Buster Makikomi Majin", // 0x25 ATTACK_ID_EM015_BUSTER_MAKIKOMI_MAJIN
    "EM025 Straight",              // 0x26 ATTACK_ID_EM025_STRAIGHT
    "EM025 Upper",                 // 0x27 ATTACK_ID_EM025_UPPER
    "EM025 Straight Left",         // 0x28 ATTACK_ID_EM025_STRAIGHT_LEFT
    "EM025 Final Flash",           // 0x29 ATTACK_ID_EM025_FINAL_FLASH
    "EM025 Hammer Both",           // 0x2A ATTACK_ID_EM025_HAMMER_BOTH
    "EM014 Enemy Block Attack",    // 0x2B ATTACK_ID_EM014_ENEMY_BLOCK_ATTACK
    "Dante Coyote",                // 0x2C ATTACK_ID_DANTE_COYOTE
    "EM025 Hammer Left",           // 0x2D ATTACK_ID_EM025_HAMMER_LEFT
    "EM025 Stage Laser",           // 0x2E ATTACK_ID_EM025_STAGE_LASER
    "EM025 Stage Cannon",          // 0x2F ATTACK_ID_EM025_STAGE_CANNON
    "Temple Blade",                // 0x30 ATTACK_ID_TEMPLE_BLADE
    "Echidna B Attack",            // 0x31 ATTACK_ID_ECHIDNA_B_ATK
    "EM025 Throw",                 // 0x32 ATTACK_ID_EM025_THROW
    "EM025 Final Flash Finish",    // 0x33 ATTACK_ID_EM025_FINAL_FLASH_FINISH
    "EM025 Death Ball",            // 0x34 ATTACK_ID_EM025_DEATH_BALL
    "EM025 Death Ball Explosion",  // 0x35 ATTACK_ID_EM025_DEATH_BALL_EXPLOSION
    "EM025 Death Ball 2",          // 0x36 ATTACK_ID_EM025_DEATH_BALL2
    "EM025 Death Ball 2 Explosion",// 0x37 ATTACK_ID_EM025_DEATH_BALL2_EXPLOSION
    "EM025 Cannon",                // 0x38 ATTACK_ID_EM025_CANNON
    "EM025 Cannon Explosion",      // 0x39 ATTACK_ID_EM025_CANNON_EXPLOSION
    "EM025 Stage Explosion",       // 0x3A ATTACK_ID_EM025_STAGE_EXPLOSION
    "EM031 Room Atk Floor",        // 0x3B ATTACK_ID_EM031_ROOM_ATK_FLOOR
    "Snatch Vergil",               // 0x3C ATTACK_ID_SNATCH_VERGIL
    "Snatch Trish",                // 0x3D ATTACK_ID_SNATCH_TRISH
    "Snatch Trish Air",            // 0x3E ATTACK_ID_SNATCH_TRISH_AIR
    "Snatch Lady",                 // 0x3F ATTACK_ID_SNATCH_LADY
    "Lady Charge Shot Lv1",        // 0x40 ATTACK_ID_LADY_CHARGE_SHOT_LV1
    "Lady Charge Shot Lv2",        // 0x41 ATTACK_ID_LADY_CHARGE_SHOT_LV2
    "Lady Charge Shot Lv3",        // 0x42 ATTACK_ID_LADY_CHARGE_SHOT_LV3
    "Thunder Brow",                // 0x43 ATTACK_ID_THUNDER_BROW
    "Corder",                      // 0x44 ATTACK_ID_COREDER
    "Lady Two-Hand Shot Lv1",      // 0x45 ATTACK_ID_LADY_TWOHAND_SHOT_LV1
    "Lady Two-Hand Shot Lv2",      // 0x46 ATTACK_ID_LADY_TWOHAND_SHOT_LV2
    "Num",                         // 0x47 ATTACK_ID_NUM
};

enum RANGE_TYPE /* int */
{
	RANGE_TYPE_MELEE = 0x0,
	RANGE_TYPE_RANGE = 0x1,
	RANGE_TYPE_GRAB = 0x2,
	RANGE_TYPE_NUM = 0x3,
};
static const char* range_type_names[] = {
    "Melee",    // 0x0
    "Ranged",   // 0x1
    "Grab",     // 0x2
    "Num"       // 0x3
};

enum DAMAGE_TYPE /* int */
{
	DAMAGE_EM_NORMAL = 0x0,
	DAMAGE_EM_COMBO = 0x1,
	DAMAGE_EM_BLOWN = 0x2,
	DAMAGE_EM_BLOWN2 = 0x3,
	DAMAGE_EM_UPPER = 0x4,
	DAMAGE_EM_GROUND = 0x5,
	DAMAGE_EM_BOUND = 0x6,
	DAMAGE_EM_GRABNEAR = 0x7,
	DAMAGE_EM_LEAN = 0x8,
	DAMAGE_PL_NORMAL = 0x9,
	DAMAGE_PL_COMBO = 0xa,
	DAMAGE_PL_WIND = 0xb,
	DAMAGE_PL_BLOWN = 0xc,
	DAMAGE_PL_UPPER = 0xd,
	DAMAGE_NUM = 0xe,
};
static const char* damage_type_names[] = {
    "Enemy - Normal",         // 0x0
    "Enemy - Combo",          // 0x1
    "Enemy - Blown",          // 0x2
    "Enemy - Blown 2",        // 0x3
    "Enemy - Upper",          // 0x4
    "Enemy - Ground",         // 0x5
    "Enemy - Bound",          // 0x6
    "Enemy - Grab Near",      // 0x7
    "Enemy - Lean",           // 0x8
    "Player - Normal",        // 0x9
    "Player - Combo",         // 0xA
    "Player - Wind",          // 0xB
    "Player - Blown",         // 0xC
    "Player - Upper",         // 0xD
    "Num"                     // 0xE
};

enum HIT_MARK_ANGLE /* int */
{
	HIT_MARK_ANGLE_FROM_UPPER = 0x0,
	HIT_MARK_ANGLE_FROM_UPPER_RIGHT = 0x1,
	HIT_MARK_ANGLE_FROM_RIGHT = 0x2,
	HIT_MARK_ANGLE_FROM_LOWER_RIGHT = 0x3,
	HIT_MARK_ANGLE_FROM_LOWER = 0x4,
	HIT_MARK_ANGLE_FROM_LOWER_LEFT = 0x5,
	HIT_MARK_ANGLE_FROM_LEFT = 0x6,
	HIT_MARK_ANGLE_FROM_UPPER_LEFT = 0x7,
	HIT_MARK_ANGLE_THRUST = 0x8,
	HIT_MARK_ANGLE_FROM_UPPER2 = 0x9,
	HIT_MARK_ANGLE_FROM_UPPER_RIGHT2 = 0xa,
	HIT_MARK_ANGLE_FROM_RIGHT2 = 0xb,
	HIT_MARK_ANGLE_FROM_LOWER_RIGHT2 = 0xc,
	HIT_MARK_ANGLE_FROM_LOWER2 = 0xd,
	HIT_MARK_ANGLE_FROM_LOWER_LEFT2 = 0xe,
	HIT_MARK_ANGLE_FROM_LEFT2 = 0xf,
	HIT_MARK_ANGLE_FROM_UPPER_LEFT2 = 0x10,
	HIT_MARK_ANGLE_THRUST2 = 0x11,
	HIT_MARK_ANGLE_FROM_UPPER3 = 0x12,
	HIT_MARK_ANGLE_FROM_UPPER_RIGHT3 = 0x13,
	HIT_MARK_ANGLE_FROM_RIGHT3 = 0x14,
	HIT_MARK_ANGLE_FROM_LOWER_RIGHT3 = 0x15,
	HIT_MARK_ANGLE_FROM_LOWER3 = 0x16,
	HIT_MARK_ANGLE_FROM_LOWER_LEFT3 = 0x17,
	HIT_MARK_ANGLE_FROM_LEFT3 = 0x18,
	HIT_MARK_ANGLE_FROM_UPPER_LEFT3 = 0x19,
	HIT_MARK_ANGLE_THRUST3 = 0x1a,
	HIT_MARK_ANGLE_BLOW_FROM_UPPER = 0x1b,
	HIT_MARK_ANGLE_BLOW_FROM_UPPER_RIGHT = 0x1c,
	HIT_MARK_ANGLE_BLOW_FROM_RIGHT = 0x1d,
	HIT_MARK_ANGLE_BLOW_FROM_LOWER_RIGHT = 0x1e,
	HIT_MARK_ANGLE_BLOW_FROM_LOWER = 0x1f,
	HIT_MARK_ANGLE_BLOW_FROM_LOWER_LEFT = 0x20,
	HIT_MARK_ANGLE_BLOW_FROM_LEFT = 0x21,
	HIT_MARK_ANGLE_BLOW_FROM_UPPER_LEFT = 0x22,
	HIT_MARK_ANGLE_BLOW_THRUST = 0x23,
	HIT_MARK_ANGLE_BLOW2_FROM_UPPER = 0x24,
	HIT_MARK_ANGLE_BLOW2_FROM_UPPER_RIGHT = 0x25,
	HIT_MARK_ANGLE_BLOW2_FROM_RIGHT = 0x26,
	HIT_MARK_ANGLE_BLOW2_FROM_LOWER_RIGHT = 0x27,
	HIT_MARK_ANGLE_BLOW2_FROM_LOWER = 0x28,
	HIT_MARK_ANGLE_BLOW2_FROM_LOWER_LEFT = 0x29,
	HIT_MARK_ANGLE_BLOW2_FROM_LEFT = 0x2a,
	HIT_MARK_ANGLE_BLOW2_FROM_UPPER_LEFT = 0x2b,
	HIT_MARK_ANGLE_BLOW2_THRUST = 0x2c,
	HIT_MARK_ANGLE_BLOW3_FROM_UPPER = 0x2d,
	HIT_MARK_ANGLE_BLOW3_FROM_UPPER_RIGHT = 0x2e,
	HIT_MARK_ANGLE_BLOW3_FROM_RIGHT = 0x2f,
	HIT_MARK_ANGLE_BLOW3_FROM_LOWER_RIGHT = 0x30,
	HIT_MARK_ANGLE_BLOW3_FROM_LOWER = 0x31,
	HIT_MARK_ANGLE_BLOW3_FROM_LOWER_LEFT = 0x32,
	HIT_MARK_ANGLE_BLOW3_FROM_LEFT = 0x33,
	HIT_MARK_ANGLE_BLOW3_FROM_UPPER_LEFT = 0x34,
	HIT_MARK_ANGLE_BLOW3_THRUST = 0x35,
	HIT_MARK_ANGLE_KICK_FROM_UPPER = 0x36,
	HIT_MARK_ANGLE_KICK_FROM_UPPER_RIGHT = 0x37,
	HIT_MARK_ANGLE_KICK_FROM_RIGHT = 0x38,
	HIT_MARK_ANGLE_KICK_FROM_LOWER_RIGHT = 0x39,
	HIT_MARK_ANGLE_KICK_FROM_LOWER = 0x3a,
	HIT_MARK_ANGLE_KICK_FROM_LOWER_LEFT = 0x3b,
	HIT_MARK_ANGLE_KICK_FROM_LEFT = 0x3c,
	HIT_MARK_ANGLE_KICK_FROM_UPPER_LEFT = 0x3d,
	HIT_MARK_ANGLE_KICK_THRUST = 0x3e,
	HIT_MARK_ANGLE_KICK2_FROM_UPPER = 0x3f,
	HIT_MARK_ANGLE_KICK2_FROM_UPPER_RIGHT = 0x40,
	HIT_MARK_ANGLE_KICK2_FROM_RIGHT = 0x41,
	HIT_MARK_ANGLE_KICK2_FROM_LOWER_RIGHT = 0x42,
	HIT_MARK_ANGLE_KICK2_FROM_LOWER = 0x43,
	HIT_MARK_ANGLE_KICK2_FROM_LOWER_LEFT = 0x44,
	HIT_MARK_ANGLE_KICK2_FROM_LEFT = 0x45,
	HIT_MARK_ANGLE_KICK2_FROM_UPPER_LEFT = 0x46,
	HIT_MARK_ANGLE_KICK2_THRUST = 0x47,
	HIT_MARK_ANGLE_KICK3_FROM_UPPER = 0x48,
	HIT_MARK_ANGLE_KICK3_FROM_UPPER_RIGHT = 0x49,
	HIT_MARK_ANGLE_KICK3_FROM_RIGHT = 0x4a,
	HIT_MARK_ANGLE_KICK3_FROM_LOWER_RIGHT = 0x4b,
	HIT_MARK_ANGLE_KICK3_FROM_LOWER = 0x4c,
	HIT_MARK_ANGLE_KICK3_FROM_LOWER_LEFT = 0x4d,
	HIT_MARK_ANGLE_KICK3_FROM_LEFT = 0x4e,
	HIT_MARK_ANGLE_KICK3_FROM_UPPER_LEFT = 0x4f,
	HIT_MARK_ANGLE_KICK3_THRUST = 0x50,
	HIT_MARK_ANGLE_NO_EFFECT = 0x51,
	HIT_MARK_ANGLE_SHOOT = 0x52,
	HIT_MARK_ANGLE_SHOOT2 = 0x53,
	HIT_MARK_ANGLE_SHOOT3 = 0x54,
	HIT_MARK_ANGLE_HIT_GROUND = 0x55,
	HIT_MARK_ANGLE_IMPACT = 0x56,
	HIT_MARK_ANGLE_IMPACT2 = 0x57,
	HIT_MARK_ANGLE_BLOW = 0x58,
	HIT_MARK_ANGLE_THUNDER = 0x59,
	HIT_MARK_ANGLE_RANDOM = 0x5a,
	HIT_MARK_ANGLE_NUM = 0x5b,
};
static const char* hit_mark_angle_names[] = {
    "From Upper",                 // 0x00
    "From Upper Right",          // 0x01
    "From Right",                // 0x02
    "From Lower Right",          // 0x03
    "From Lower",                // 0x04
    "From Lower Left",           // 0x05
    "From Left",                 // 0x06
    "From Upper Left",           // 0x07
    "Thrust",                    // 0x08
    "From Upper 2",              // 0x09
    "From Upper Right 2",        // 0x0A
    "From Right 2",              // 0x0B
    "From Lower Right 2",        // 0x0C
    "From Lower 2",              // 0x0D
    "From Lower Left 2",         // 0x0E
    "From Left 2",               // 0x0F
    "From Upper Left 2",         // 0x10
    "Thrust 2",                  // 0x11
    "From Upper 3",              // 0x12
    "From Upper Right 3",        // 0x13
    "From Right 3",              // 0x14
    "From Lower Right 3",        // 0x15
    "From Lower 3",              // 0x16
    "From Lower Left 3",         // 0x17
    "From Left 3",               // 0x18
    "From Upper Left 3",         // 0x19
    "Thrust 3",                  // 0x1A
    "Blow From Upper",           // 0x1B
    "Blow From Upper Right",     // 0x1C
    "Blow From Right",           // 0x1D
    "Blow From Lower Right",     // 0x1E
    "Blow From Lower",           // 0x1F
    "Blow From Lower Left",      // 0x20
    "Blow From Left",            // 0x21
    "Blow From Upper Left",      // 0x22
    "Blow Thrust",               // 0x23
    "Blow2 From Upper",          // 0x24
    "Blow2 From Upper Right",    // 0x25
    "Blow2 From Right",          // 0x26
    "Blow2 From Lower Right",    // 0x27
    "Blow2 From Lower",          // 0x28
    "Blow2 From Lower Left",     // 0x29
    "Blow2 From Left",           // 0x2A
    "Blow2 From Upper Left",     // 0x2B
    "Blow2 Thrust",              // 0x2C
    "Blow3 From Upper",          // 0x2D
    "Blow3 From Upper Right",    // 0x2E
    "Blow3 From Right",          // 0x2F
    "Blow3 From Lower Right",    // 0x30
    "Blow3 From Lower",          // 0x31
    "Blow3 From Lower Left",     // 0x32
    "Blow3 From Left",           // 0x33
    "Blow3 From Upper Left",     // 0x34
    "Blow3 Thrust",              // 0x35
    "Kick From Upper",           // 0x36
    "Kick From Upper Right",     // 0x37
    "Kick From Right",           // 0x38
    "Kick From Lower Right",     // 0x39
    "Kick From Lower",           // 0x3A
    "Kick From Lower Left",      // 0x3B
    "Kick From Left",            // 0x3C
    "Kick From Upper Left",      // 0x3D
    "Kick Thrust",               // 0x3E
    "Kick2 From Upper",          // 0x3F
    "Kick2 From Upper Right",    // 0x40
    "Kick2 From Right",          // 0x41
    "Kick2 From Lower Right",    // 0x42
    "Kick2 From Lower",          // 0x43
    "Kick2 From Lower Left",     // 0x44
    "Kick2 From Left",           // 0x45
    "Kick2 From Upper Left",     // 0x46
    "Kick2 Thrust",              // 0x47
    "Kick3 From Upper",          // 0x48
    "Kick3 From Upper Right",    // 0x49
    "Kick3 From Right",          // 0x4A
    "Kick3 From Lower Right",    // 0x4B
    "Kick3 From Lower",          // 0x4C
    "Kick3 From Lower Left",     // 0x4D
    "Kick3 From Left",           // 0x4E
    "Kick3 From Upper Left",     // 0x4F
    "Kick3 Thrust",              // 0x50
    "No Effect",                 // 0x51
    "Shoot",                     // 0x52
    "Shoot 2",                   // 0x53
    "Shoot 3",                   // 0x54
    "Hit Ground",                // 0x55
    "Impact",                    // 0x56
    "Impact 2",                  // 0x57
    "Blow",                      // 0x58
    "Thunder",                   // 0x59
    "Random",                    // 0x5A
    "Num"                        // 0x5B
};

enum ATTACK_FLAG /* int */
{
	ATTACK_FLAG_DEFAULT = 0x0,
	ATTACK_FLAG_NO_DEATH = 0x1,
	ATTACK_FLAG_NUM = 0x2,
};
static const char* attack_flag_names[] = {
    "Default",     // 0x0
    "No Death",    // 0x1
    "Num"        // 0x2
};

enum BLOWN_ANGLE_TYPE /* int */
{
	BLOWN_ANGLE_TYPE_POS = 0x0,
	BLOWN_ANGLE_TYPE_ANG = 0x1,
	BLOWN_ANGLE_TYPE_ANG_INV = 0x2,
	BLOWN_ANGLE_TYPE_POS_INV = 0x3,
	BLOWN_ANGLE_TYPE_PLAYER = 0x4,
	BLOWN_ANGLE_TYPE_PLAYER_INV = 0x5,
	BLOWN_ANGLE_TYPE_NUM = 0x6,
};
static const char* blown_angle_type_names[] = {
    "Position",            // 0x0
    "Angle",               // 0x1
    "Angle Inverted",      // 0x2
    "Position Inverted",   // 0x3
    "From Player",         // 0x4
    "From Player Inv.",    // 0x5
    "Num"                  // 0x6
};

enum ELEMENT_TYPE_DMC4 /* int */
{
	ELEMENT_TYPE_NONE = 0x0,
	ELEMENT_TYPE_FIRE = 0x1,
	ELEMENT_TYPE_FIRE2 = 0x2,
	ELEMENT_TYPE_FIRE3 = 0x3,
	ELEMENT_TYPE_ICE = 0x4,
	ELEMENT_TYPE_THUNDER = 0x5,
	ELEMENT_TYPE_ECHIDNA = 0x6,
	ELEMENT_TYPE_NUM = 0x7,
};
static const char* element_type_dmc4_names[] = {
    "None",          // 0x0
    "Fire",          // 0x1
    "Fire2",         // 0x2
    "Fire3",         // 0x3
    "Ice",           // 0x4
    "Thunder",       // 0x5
    "Echidna",       // 0x6
    "Num",           // 0x7
};

struct kAttackStatus_v3 
{
	char mAsName[16]; // 0x00000000
	//int mAttackIDProp; // 0x00000010
	ATTACK_ID mAttackID; // 0x00000010
	float mDamageValue; // 0x00000014
	int mAttackLv; // 0x00000018
	int mAttackLvI; // 0x0000001c
	int mAttackLvB; // 0x00000020
	//int mRangeTypeProp; // 0x00000024
	RANGE_TYPE mRangeType; // 0x00000024
	int mHitStopTimer; // 0x00000028
	DAMAGE_TYPE mDamageType; // 0x0000002c
	int mDamageTypeI; // 0x00000030
	int mDamageTypeB; // 0x00000034
	//int mHitMarkAngleProp; // 0x00000038
	HIT_MARK_ANGLE mHitMarkAngle; // 0x00000038
	unsigned int mHitSE; // 0x0000003c
	float mStylishPoint; // 0x00000040
	float mStylishTimer; // 0x00000044
	float mDTAdd; // 0x00000048
	//int mAttackFlagProp; // 0x0000004c
	ATTACK_FLAG mAttackFlag; // 0x0000004c
	//int mBlownAngleTypeProp; // 0x00000050
	BLOWN_ANGLE_TYPE mBlownAngleType; // 0x00000050
	//int mElementTypeProp; // 0x00000054
	ELEMENT_TYPE_DMC4 mElementType; // 0x00000054
};
