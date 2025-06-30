#pragma once
/*#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

//#include "glm/mat3x3.hpp"

using Vector3f = glm::vec3;
using Vector4 = glm::vec4;

// Generated using ReClass 2016
#include <cstdint>*/
#ifndef RECLASS_INTERNAL_HPP
#define RECLASS_INTERNAL_HPP
#endif

#include <vector>
#include <string>
#include ".\utility\MoFile.hpp"
#include "MtMath.hpp"
#define _(string) utility::text_lookup(string)
#define __(str) str

enum class GameDifficulty : uint32_t {
    HUMAN                 = 0,
    DEVIL_HUNTER          = 1,
    SON_OF_SPARDA_HOH_HAH = 2,
    DANTE_MUST_DIE        = 3,
    LEGENDARY_DARK_KNIGHT = 4,
};

enum class GameMode : uint32_t {
    DEFAULT        = 0,
    HEAVEN_OR_HELL = 1,
    HELL_AND_HELL  = 2,
};

static std::vector<std::pair<uint16_t, const char*>> buttonPairs = {
    {0x0000, __("None")},
    {0x0001, __("Select")},
    {0x0002, __("L3")},
    {0x0004, __("R3")},
    {0x0008, __("Start")},
    {0x0010, __("Dpad Up")},
    {0x0020, __("Dpad Right")},
    {0x0040, __("Dpad Down")},
    {0x0080, __("Dpad Left")},
    {0x0100, __("L1")},
    {0x0200, __("R1")},
    {0x0400, __("L2")},
    {0x0800, __("R2")},
    {0x1000, __("Y")},
    {0x2000, __("B")},
    {0x4000, __("A")},
    {0x8000, __("X")}
};

enum ATTACK_DIRECTION_CMD {
    LOCKON_MELEE               = 0,
    LOCKON_GUN                 = 1,
    LOCKON_STYLE               = 2,
    LOCKON_FORWARD_MELEE       = 3,
    LOCKON_BACK_MELEE          = 4,
    LOCKON_FORWARD_GUN         = 5,
    LOCKON_BACK_GUN            = 6,
    LOCKON_FORWARD_STYLE       = 7,
    LOCKON_BACK_STYLE          = 8,
    LOCKON_FORWARD_MELEE2      = 9,
    LOCKON_BACK_MELEE2         = 10,
    LOCKON_FORWARD_GUN2        = 11,
    LOCKON_BACK_GUN2           = 12,
    LOCKON_FORWARD_STYLE2      = 13,
    LOCKON_BACK_STYLE2         = 14,
    LOCKON_FORWARD_MELEE_STICK = 15,
    LOCKON_BACK_MELEE_STICK    = 16,
    LOCKON_FORWARD_GUN_STICK   = 17,
    LOCKON_BACK_GUN_STICK      = 18,
    LOCKON_FORWARD_STYLE_STICK = 19,
    LOCKON_BACK_STYLE_STICK    = 20,
    LOCKON_ANYDIRECTION_MELEE  = 21,
    LOCKON_ANYDIRECTION_GUN    = 22,
    LOCKON_ANYDIRECTION_STYLE  = 23,
    ANYDIRECTION_MELEE         = 24,
    ANYDIRECTION_GUN           = 25,
    ANYDIRECTION_STYLE         = 26,
    // ...
    GUN_RELEASE         = 32,
    GUN_CHARGED_RELEASE = 33,
    LOCKON_GUN_CHARGE   = 34,
    GUN_CHARGE_2OR3     = 35,
    GUN_CHARGE3         = 36,
    GUN_CHARGE4         = 37,
    GUN_CHARGE_3        = 38,
    // ...
    MELEE_STYLE              = 81,
    LOCKON_BACKFORWARD_MELEE = 82,
    // 83
    GUN = 84
};

enum class WEIGHT_TYPE : uint32_t {
  WT_NO_WEIGHT = 0x0,
  WT_LIGHT0 = 0x1,
  WT_LIGHT1 = 0x2,
  WT_LIGHT2 = 0x3,
  WT_LIGHT3 = 0x4,
  WT_MIDDLE0 = 0x5,
  WT_MIDDLE1 = 0x6,
  WT_MIDDLE2 = 0x7,
  WT_MIDDLE3 = 0x8,
  WT_HEAVY0 = 0x64,
  WT_HEAVY1 = 0x78,
  WT_HEAVY2 = 0x8C,
  WT_HEAVY3 = 0xA0,
  WT_MAX_WEIGHT = 0x3E8,
  WT_MAX_WEIGHT2 = 0x2710,
  WT_MAX = 0x2711,
};

enum class COLLISION_ACTOR_TYPE : uint32_t {
  PL = 0x0,
  PS = 0x1,
  EM = 0x2,
  ES = 0x3,
  SET = 0x4,
  STG = 0x5,
  MAX_COLLISION_ACTOR_TYPE = 0x6,
};

enum ENEMY_ID : int {
  SCARECROW_LEG = 0, // Legcrow
  SCARECROW_ARM = 1, // Armcrow
  EM002 = 2,
  SCARECROW_MEGA = 3, // Mega
  EM004 = 4,
  ANGELO_BIANCO = 5,
  ANGELO_ALTO = 6,
  EM007 = 7,
  MEPHISTO = 8,
  FAUST = 9,
  FROST = 10,
  ASSAULT = 11,
  BLITZ = 12,
  CHIMERA = 13,
  EM014 = 14,
  CUTLASS = 15,
  GLADIUS = 16,
  BASILISK = 17,
  BERIAL = 18,
  BAEL = 19,
  EM020 = 20,
  ECHIDNA = 21,
  CREDO = 22,
  AGNUS = 23,
  EM024 = 24,
  SAVIOR = 25,
  EM026 = 26,
  EM027 = 27,
  EM028 = 28,
  SANCTUS_M11 = 29,
  SANCTUS_M20 = 30,
};

enum DANTE_ATCK_ID {              
	ATCK_ID_RV_COMBO0 = 0x1,                // Rebellion Combo A1
	ATCK_ID_RV_COMBO1 = 0x2,                // Rebellion Combo A2
	ATCK_ID_RV_COMBO2 = 0x3,                // Rebellion Combo A3
	ATCK_ID_RV_DELAY_COMBO0 = 0x4,          // Rebellion Combo B1
	ATCK_ID_RV_DELAY_COMBO1 = 0x5,          // Rebellion Combo B2
	ATCK_ID_RV_HYAKURETSU = 0x6,            // Million Stab
	ATCK_ID_RV_KABUTO = 0x7,                // Helm Breaker
	ATCK_ID_RV_STINGER_LV1 = 0x8,           // Stinger Level 1
	ATCK_ID_RV_STINGER_LV2 = 0x9,           // Stinger Level 2
	ATCK_ID_RV_DT_STINGER = 0xa,            // Stinger DT
	ATCK_ID_RV_HITIME = 0xb,                // High Time (Ground)
	ATCK_ID_RV_HITIMEJUMP = 0xc,            // High Time (Air)
	ATCK_ID_RV_ROUND_TRIP = 0xd,            // Round Trip
	ATCK_ID_RV_RETURN = 0xe,                // Round Trip (Call Back Ground)
	ATCK_ID_RV_RETURN_AIR = 0xf, 	        // Round Trip (Call Back Air)
	ATCK_ID_SRV_ROLLING = 0x10,             // Prop
	ATCK_ID_SRV_ROLLING_SERIES = 0x11,      // Shredder
	ATCK_ID_SRV_AIR_COMBO0 = 0x12, 	        // Aerial Rave 1
	ATCK_ID_SRV_AIR_COMBO1 = 0x13, 	        // Aerial Rave 2
	ATCK_ID_SRV_AIR_COMBO2 = 0x14, 	        // Aerial Rave 3
	ATCK_ID_SRV_AIR_COMBO3 = 0x15, 	        // Aerial Rave 4
	ATCK_ID_SRV_DANCE = 0x16, 	 	        // Dance Macabre (Starting from RebA1)
	ATCK_ID_SRV_DRIVE = 0x17, 		        // Drive
	ATCK_ID_SRV_DRIVE_SERIES = 0x18,        // Overdrive
	ATCK_ID_SRV_COMBO0_DRIVE = 0x19,        // Quick Drive

	ATCK_ID_GM_L_STRAIGHT = 0x1a, 		    // Gilgamesh Combo A1
	ATCK_ID_GM_R_BODY = 0x1b,			    // Gilgamesh Combo A2
	ATCK_ID_GM_HI_KICK = 0x1c,			    // Gilgamesh Combo A3
	ATCK_ID_GM_HEEL = 0x1d,				    // Gilgamesh Combo A4
	ATCK_ID_GM_HYAKURETSU = 0x1e,		    // Gilgamesh Combo B1
	ATCK_ID_GM_WHIRLWIND = 0x1f,		    // Gilgamesh Combo B2
	ATCK_ID_GM_DESCENT_KICK = 0x20,		    // Full House
	ATCK_ID_GM_KICK_13R = 0x21,			    // Kick 13 (Regular)
	ATCK_ID_GM_SWAYING_BACK = 0x22, 	    // Draw
	ATCK_ID_GM_STRAIGHT = 0x23,			    // Straight 
	ATCK_ID_SGM_INFERNO = 0x24,  		    // Shock (Ground)
	ATCK_ID_SGM_AIR_INFERNO = 0x25,		    // Shocking (Air)
	ATCK_ID_SGM_REAL_IMPACT = 0x26,	  	    // Real Impact 
	ATCK_ID_SGM_MAGMA_DRIVE = 0x27,		    // Beast Uppercut
	ATCK_ID_SGM_RISING_DRAGON = 0x28,  	    // Rising Dragon 
	ATCK_ID_SGM_DIVINE_DRAGON = 0x29,  	    // Divine Dragon 

	ATCK_ID_LF_WIDE_RANGE_COMBO0 = 0x2a,    // Lucifer Combo A1 
	ATCK_ID_LF_WIDE_RANGE_COMBO1 = 0x2b,    // Lucifer Combo A2 
	ATCK_ID_LF_WIDE_RANGE_COMBO2 = 0x2c,    // Lucifer Combo A3 
	ATCK_ID_LF_WIDE_RANGE_COMBO3 = 0x2d,    // Lucifer Combo A4 
	ATCK_ID_LF_CROSS_THROW = 0x2e,          // Lucifer Combo D1
	ATCK_ID_LF_BOISTEROUS = 0x2f,           // Lucifer Combo C1
	ATCK_ID_LF_LAUNCH = 0x30,               // Lucifer Combo E1
	ATCK_ID_LF_ROLLING = 0x31,              // Lucifer Combo B1
	ATCK_ID_LF_ARRANGEMENT_COMBO = 0x32,    // N/A
	ATCK_ID_LF_AIR_ARRANGEMENT = 0x33,      // Splash
	ATCK_ID_LF_DIO_KNIFE = 0x34,            // Pin Up (Regular)
	ATCK_ID_LF_ADD_DIO_KNIFE = 0x35,        // Pin Up (DT)
	ATCK_ID_LF_DETONATE_ALL = 0x36,         // Ecstasy (Ground)
	ATCK_ID_LF_AIR_DETONATE_ALL = 0x37,     // Ecstasy (Air)
	ATCK_ID_SLF_BOM_BARRIER = 0x38,         // Climax (Ground)
	ATCK_ID_SLF_AIR_BOM_BARRIER = 0x39,     // Climax (Air)
	ATCK_ID_SLF_FUNNEL = 0x3a,              // Discipline
	ATCK_ID_SLF_CONCENTRATED_LAUNCH = 0x3b, // Bondage?

	ATCK_ID_EI_SHOT = 0x3c,                 // Ebony & Ivory Shoot (Ground)
	ATCK_ID_EI_CSHOT_LV1 = 0x3d,            // Ebony & Ivory Charged Shot Lv1 (Ground)
	ATCK_ID_EI_CSHOT_LV2 = 0x3e,            // Ebony & Ivory Charged Shot Lv2 (Ground)
	ATCK_ID_EI_CSHOT_LV3 = 0x3f,            // Ebony & Ivory Charged Shot Lv3 (Ground)
	ATCK_ID_EI_AIR_SHOT = 0x40,             // Ebony & Ivory Shoot (Air)
	ATCK_ID_EI_AIR_CSHOT_LV1 = 0x41,        // Ebony & Ivory Charged Shot Lv1 (Air)
	ATCK_ID_EI_AIR_CSHOT_LV2 = 0x42,        // Ebony & Ivory Charged Shot Lv2 (Air)
	ATCK_ID_EI_AIR_CSHOT_LV3 = 0x43,        // Ebony & Ivory Charged Shot Lv3 (Air)
	ATCK_ID_SEI_TWO_SOMETIME = 0x44,        // Ebony & Ivory Twosome Time
	ATCK_ID_SEI_HONEYCOMB = 0x45,           // Ebony & Ivory Honeycomb Fire
	ATCK_ID_SEI_RAIN_STORM = 0x46,          // Ebony & Ivory Rainstorm 

	ATCK_ID_CA_SHOT = 0x47,                 // Coyote Ace Shoot (Ground)
	ATCK_ID_CA_CSHOT_LV1 = 0x48,            // Coyote Ace Charged Shot Lv1 (Ground)
	ATCK_ID_CA_CSHOT_LV2 = 0x49,            // Coyote Ace Charged Shot Lv2 (Ground)
	ATCK_ID_CA_CSHOT_LV3 = 0x4a,            // Coyote Ace Charged Shot Lv3 (Ground)
	ATCK_ID_CA_AIR_SHOT = 0x4b,             // Coyote Ace Shoot (Air)
	ATCK_ID_CA_AIR_CSHOT_LV1 = 0x4c,        // Coyote Ace Charged Shot Lv1 (Air)
	ATCK_ID_CA_AIR_CSHOT_LV2 = 0x4d,        // Coyote Ace Charged Shot Lv2 (Air)
	ATCK_ID_CA_AIR_CSHOT_LV3 = 0x4e,        // Coyote Ace Charged Shot Lv3 (Air)
	ATCK_ID_SCA_NUNCHAKU = 0x4f,            // Coyote Ace Fireworks (Ground)
	ATCK_ID_SCA_AIR_NUNCHAKU = 0x50,        // Coyote Ace Fireworks (Air)
	ATCK_ID_SCA_BACK_SHOT = 0x51,           // Coyote Ace Backslide
	ATCK_ID_SCA_STINGER = 0x52,             // Coyote Ace Gun Stinger

	ATCK_ID_PD_LAUNCHER = 0x53,             // PF013: Epidemic (Bow Gun)
	ATCK_ID_PD_LAUNCHER2 = 0x54,            // PF124: Hatred (Rocket Launcher)
	ATCK_ID_PD_LAUNCHER3 = 0x55,            // PF398: Revenge (Laser Cannon)
	ATCK_ID_PD_AIR_MACHINE_GUN = 0x56,      // PF262: Jealousy
	ATCK_ID_SPD_BATTERY = 0x57,             // N/A
	ATCK_ID_SPD_METEOR = 0x58,              // PF594: Argument
	ATCK_ID_SPD_BOOMERANG = 0x59,           // PF422: Grief 
	ATCK_ID_SPD_PANDORA_BOX = 0x5a,         // PF666: Omen

	ATCK_ID_TS_DASH = 0x5b,                 // Trickster Dash 
	ATCK_ID_TS_AIR_DASH = 0x5c,             // Sky Star 
	ATCK_ID_TS_AIRTRICK = 0x5d,             // Air Trick
	ATCK_ID_TS_BACK_WARP = 0x5e,            // Flipper?

	ATCK_ID_RG_RELEASE = 0x5f,              // Release (Bronze No Target)
	ATCK_ID_RG_GUARD_RELEASE = 0x60,        // Release (Silver Target missed timing)
	ATCK_ID_RG_JUST_RELEASE = 0x61,         // Release (Gold Royal Release)
	ATCK_ID_RG_AIR_RELEASE = 0x62,          // Release Air (Bronze No Target) 
	ATCK_ID_RG_AIR_GUARD_RELEASE = 0x63,    // Release Air (Silver Target missed timing)
	ATCK_ID_RG_AIR_JUST_RELEASE = 0x64,     // Release Air (Gold Royal Release)
	ATCK_ID_RG_INVINCIBLE = 0x65,           // Dreadnaught

	ATCK_ID_YT_COMBO0 = 0x66,               // Yamato Combo A1
	ATCK_ID_YT_COMBO1 = 0x67,               // Yamato Combo A2
	ATCK_ID_YT_COMBO2 = 0x68,               // Yamato Combo A3
	ATCK_ID_YT_AIR_COMBO0 = 0x69,           // Aerial Rave V1
	ATCK_ID_YT_AIR_COMBO1 = 0x6a,           // Aerial Rave V2 
	ATCK_ID_YT_SPACE = 0x6b,                // Slash Dimension (Far)
	ATCK_ID_YT_SPACE2 = 0x6c,               // Slash Dimension (Close)
	ATCK_ID_MAX = 0x6d,
};

static const char* dante_attack_names[109] = {
    "",                                      // 0x00 /   0 
    __("Rebellion Combo A1"),                   // 0x01 /   1  ATCK_ID_RV_COMBO0
    __("Rebellion Combo A2"),                   // 0x02 /   2  ATCK_ID_RV_COMBO1
    __("Rebellion Combo A3"),                   // 0x03 /   3  ATCK_ID_RV_COMBO2
    __("Rebellion Combo B1"),                   // 0x04 /   4  ATCK_ID_RV_DELAY_COMBO0
    __("Rebellion Combo B2"),                   // 0x05 /   5  ATCK_ID_RV_DELAY_COMBO1
    __("Million Stab"),                         // 0x06 /   6  ATCK_ID_RV_HYAKURETSU
    __("Helm Breaker"),                         // 0x07 /   7  ATCK_ID_RV_KABUTO
    __("Stinger Level 1"),                      // 0x08 /   8  ATCK_ID_RV_STINGER_LV1
    __("Stinger Level 2"),                      // 0x09 /   9  ATCK_ID_RV_STINGER_LV2
    __("Stinger DT"),                           // 0x0A /  10  ATCK_ID_RV_DT_STINGER
    __("High Time (Ground)"),                   // 0x0B /  11  ATCK_ID_RV_HITIME
    __("High Time (Air)"),                      // 0x0C /  12  ATCK_ID_RV_HITIMEJUMP
    __("Round Trip"),                           // 0x0D /  13  ATCK_ID_RV_ROUND_TRIP
    __("Round Trip (Call Back Ground)"),        // 0x0E /  14  ATCK_ID_RV_RETURN
    __("Round Trip (Call Back Air)"),           // 0x0F /  15  ATCK_ID_RV_RETURN_AIR
    __("Prop"),                                 // 0x10 /  16  ATCK_ID_SRV_ROLLING
    __("Shredder"),                             // 0x11 /  17  ATCK_ID_SRV_ROLLING_SERIES
    __("Aerial Rave 1"),                        // 0x12 /  18  ATCK_ID_SRV_AIR_COMBO0
    __("Aerial Rave 2"),                        // 0x13 /  19  ATCK_ID_SRV_AIR_COMBO1
    __("Aerial Rave 3"),                        // 0x14 /  20  ATCK_ID_SRV_AIR_COMBO2
    __("Aerial Rave 4"),                        // 0x15 /  21  ATCK_ID_SRV_AIR_COMBO3
    __("Dance Macabre"),                        // 0x16 /  22  ATCK_ID_SRV_DANCE
    __("Drive"),                                // 0x17 /  23  ATCK_ID_SRV_DRIVE
    __("Overdrive"),                            // 0x18 /  24  ATCK_ID_SRV_DRIVE_SERIES
    __("Quick Drive"),                          // 0x19 /  25  ATCK_ID_SRV_COMBO0_DRIVE
    __("Gilgamesh Combo A1"),                   // 0x1A /  26  ATCK_ID_GM_L_STRAIGHT
    __("Gilgamesh Combo A2"),                   // 0x1B /  27  ATCK_ID_GM_R_BODY
    __("Gilgamesh Combo A3"),                   // 0x1C /  28  ATCK_ID_GM_HI_KICK
    __("Gilgamesh Combo A4"),                   // 0x1D /  29  ATCK_ID_GM_HEEL
    __("Gilgamesh Combo B1"),                   // 0x1E /  30  ATCK_ID_GM_HYAKURETSU
    __("Gilgamesh Combo B2"),                   // 0x1F /  31  ATCK_ID_GM_WHIRLWIND
    __("Full House"),                           // 0x20 /  32  ATCK_ID_GM_DESCENT_KICK
    __("Kick 13 (Regular)"),                    // 0x21 /  33  ATCK_ID_GM_KICK_13R
    __("Draw"),                                 // 0x22 /  34  ATCK_ID_GM_SWAYING_BACK
    __("Straight"),                             // 0x23 /  35  ATCK_ID_GM_STRAIGHT
    __("Shock (Ground)"),                       // 0x24 /  36  ATCK_ID_SGM_INFERNO
    __("Shocking (Air)"),                       // 0x25 /  37  ATCK_ID_SGM_AIR_INFERNO
    __("Real Impact"),                          // 0x26 /  38  ATCK_ID_SGM_REAL_IMPACT
    __("Beast Uppercut"),                       // 0x27 /  39  ATCK_ID_SGM_MAGMA_DRIVE
    __("Rising Dragon"),                        // 0x28 /  40  ATCK_ID_SGM_RISING_DRAGON
    __("Divine Dragon"),                        // 0x29 /  41  ATCK_ID_SGM_DIVINE_DRAGON
    __("Lucifer Combo A1"),                     // 0x2A /  42  ATCK_ID_LF_WIDE_RANGE_COMBO0
    __("Lucifer Combo A2"),                     // 0x2B /  43  ATCK_ID_LF_WIDE_RANGE_COMBO1
    __("Lucifer Combo A3"),                     // 0x2C /  44  ATCK_ID_LF_WIDE_RANGE_COMBO2
    __("Lucifer Combo A4"),                     // 0x2D /  45  ATCK_ID_LF_WIDE_RANGE_COMBO3
    __("Lucifer Combo D1"),                     // 0x2E /  46  ATCK_ID_LF_CROSS_THROW
    __("Lucifer Combo C1"),                     // 0x2F /  47  ATCK_ID_LF_BOISTEROUS
    __("Lucifer Combo E1"),                     // 0x30 /  48  ATCK_ID_LF_LAUNCH
    __("Lucifer Combo B1"),                     // 0x31 /  49  ATCK_ID_LF_ROLLING
    __("ATCK_ID_LF_ARRANGEMENT_COMBO"),         // 0x32 /  50  ATCK_ID_LF_ARRANGEMENT_COMBO
    __("Splash"),                               // 0x33 /  51  ATCK_ID_LF_AIR_ARRANGEMENT
    __("Pin Up (Regular)"),                     // 0x34 /  52  ATCK_ID_LF_DIO_KNIFE
    __("Pin Up (DT)"),                          // 0x35 /  53  ATCK_ID_LF_ADD_DIO_KNIFE
    __("Ecstasy (Ground)"),                     // 0x36 /  54  ATCK_ID_LF_DETONATE_ALL
    __("Ecstasy (Air)"),                        // 0x37 /  55  ATCK_ID_LF_AIR_DETONATE_ALL
    __("Climax (Ground)"),                      // 0x38 /  56  ATCK_ID_SLF_BOM_BARRIER
    __("Climax (Air)"),                         // 0x39 /  57  ATCK_ID_SLF_AIR_BOM_BARRIER
    __("Discipline"),                           // 0x3A /  58  ATCK_ID_SLF_FUNNEL
    __("Bondage?"),                             // 0x3B /  59  ATCK_ID_SLF_CONCENTRATED_LAUNCH
    __("Ebony & Ivory Shoot (Ground)"),         // 0x3C /  60  ATCK_ID_EI_SHOT
    __("Ebony & Ivory Charged Shot Lv1 (G)"),   // 0x3D /  61  ATCK_ID_EI_CSHOT_LV1
    __("Ebony & Ivory Charged Shot Lv2 (G)"),   // 0x3E /  62  ATCK_ID_EI_CSHOT_LV2
    __("Ebony & Ivory Charged Shot Lv3 (G)"),   // 0x3F /  63  ATCK_ID_EI_CSHOT_LV3
    __("Ebony & Ivory Shoot (Air)"),            // 0x40 /  64  ATCK_ID_EI_AIR_SHOT
    __("Ebony & Ivory Charged Shot Lv1 (A)"),   // 0x41 /  65  ATCK_ID_EI_AIR_CSHOT_LV1
    __("Ebony & Ivory Charged Shot Lv2 (A)"),   // 0x42 /  66  ATCK_ID_EI_AIR_CSHOT_LV2
    __("Ebony & Ivory Charged Shot Lv3 (A)"),   // 0x43 /  67  ATCK_ID_EI_AIR_CSHOT_LV3
    __("Ebony & Ivory Twosome Time"),           // 0x44 /  68  ATCK_ID_SEI_TWO_SOMETIME
    __("Ebony & Ivory Honeycomb Fire"),         // 0x45 /  69  ATCK_ID_SEI_HONEYCOMB
    __("Ebony & Ivory Rainstorm"),              // 0x46 /  70  ATCK_ID_SEI_RAIN_STORM
    __("Coyote Ace Shoot (Ground)"),            // 0x47 /  71  ATCK_ID_CA_SHOT
    __("Coyote Ace Charged Shot Lv1 (G)"),      // 0x48 /  72  ATCK_ID_CA_CSHOT_LV1
    __("Coyote Ace Charged Shot Lv2 (G)"),      // 0x49 /  73  ATCK_ID_CA_CSHOT_LV2
    __("Coyote Ace Charged Shot Lv3 (G)"),      // 0x4A /  74  ATCK_ID_CA_CSHOT_LV3
    __("Coyote Ace Shoot (Air)"),               // 0x4B /  75  ATCK_ID_CA_AIR_SHOT
    __("Coyote Ace Charged Shot Lv1 (A)"),      // 0x4C /  76  ATCK_ID_CA_AIR_CSHOT_LV1
    __("Coyote Ace Charged Shot Lv2 (A)"),      // 0x4D /  77  ATCK_ID_CA_AIR_CSHOT_LV2
    __("Coyote Ace Charged Shot Lv3 (A)"),      // 0x4E /  78  ATCK_ID_CA_AIR_CSHOT_LV3
    __("Coyote Ace Fireworks (Ground)"),        // 0x4F /  79  ATCK_ID_SCA_NUNCHAKU
    __("Coyote Ace Fireworks (Air)"),           // 0x50 /  80  ATCK_ID_SCA_AIR_NUNCHAKU
    __("Coyote Ace Backslide"),                 // 0x51 /  81  ATCK_ID_SCA_BACK_SHOT
    __("Coyote Ace Gun Stinger"),               // 0x52 /  82  ATCK_ID_SCA_STINGER
    __("PF013: Epidemic"),                      // 0x53 /  83  ATCK_ID_PD_LAUNCHER
    __("PF124: Hatred"),                        // 0x54 /  84  ATCK_ID_PD_LAUNCHER2
    __("PF398: Revenge"),                       // 0x55 /  85  ATCK_ID_PD_LAUNCHER3
    __("PF262: Jealousy"),                      // 0x56 /  86  ATCK_ID_PD_AIR_MACHINE_GUN
    __("ATCK_ID_SPD_BATTERY"),                  // 0x57 /  87  ATCK_ID_SPD_BATTERY
    __("PF594: Argument"),                      // 0x58 /  88  ATCK_ID_SPD_METEOR
    __("PF422: Grief"),                         // 0x59 /  89  ATCK_ID_SPD_BOOMERANG
    __("PF666: Omen"),                          // 0x5A /  90  ATCK_ID_SPD_PANDORA_BOX
    __("Trickster Dash"),                       // 0x5B /  91  ATCK_ID_TS_DASH
    __("Sky Star"),                             // 0x5C /  92  ATCK_ID_TS_AIR_DASH
    __("Air Trick"),                            // 0x5D /  93  ATCK_ID_TS_AIRTRICK
    __("Flipper?"),                             // 0x5E /  94  ATCK_ID_TS_BACK_WARP
    __("Release (Bronze No Target)"),           // 0x5F /  95  ATCK_ID_RG_RELEASE
    __("Release (Silver Missed)"),              // 0x60 /  96  ATCK_ID_RG_GUARD_RELEASE
    __("Release (Gold Just)"),                  // 0x61 /  97  ATCK_ID_RG_JUST_RELEASE
    __("Release Air (Bronze)"),                 // 0x62 /  98  ATCK_ID_RG_AIR_RELEASE
    __("Release Air (Silver)"),                 // 0x63 /  99  ATCK_ID_RG_AIR_GUARD_RELEASE
    __("Release Air (Gold)"),                   // 0x64 / 100  ATCK_ID_RG_AIR_JUST_RELEASE
    __("Dreadnaught"),                          // 0x65 / 101  ATCK_ID_RG_INVINCIBLE
    __("Yamato Combo A1"),                      // 0x66 / 102  ATCK_ID_YT_COMBO0
    __("Yamato Combo A2"),                      // 0x67 / 103  ATCK_ID_YT_COMBO1
    __("Yamato Combo A3"),                      // 0x68 / 104  ATCK_ID_YT_COMBO2
    __("Aerial Rave V1"),                       // 0x69 / 105  ATCK_ID_YT_AIR_COMBO0
    __("Aerial Rave V2"),                       // 0x6A / 106  ATCK_ID_YT_AIR_COMBO1
    __("Slash Dimension (Far)"),                // 0x6B / 107  ATCK_ID_YT_SPACE
    __("Slash Dimension (Close)"),              // 0x6C / 108  ATCK_ID_YT_SPACE2
};

static const char* directionMappingNames[] = {
    __("0 - None / Lockon + Melee"),
    __("1 - Lockon + Gun"),
    __("2 - Lockon + Style"),
    __("3 - Lockon + Forward + Melee"),
    __("4 - Lockon + Back + Melee"),
    __("5 - Lockon + Forward + Gun"),
    __("6 - Lockon + Back + Gun"), 
    __("7 - Lockon + Forward + Style"),
    __("8 - Lockon + Back + Style"),
    __("9 - Lockon + Forward + Melee2"),
    __("10 - Lockon + Back + Melee2"),
    __("11 - Lockon + Forward + Gun2"),
    __("12 - Lockon + Back + Gun2"),
    __("13 - Lockon + Forward + Style2"),
    __("14 - Lockon + Back + Style2"),
    __("15 - Lockon + Forward + Melee Stick"),
    __("16 - Lockon + Back + Melee Stick"),
    __("17 - Lockon + Forward + Gun Stick"),
    __("18 - Lockon + Back + Gun Stick"),
    __("19 - Lockon + Forward + Style Stick"), 
    __("20 - Lockon + Back + Style Stick"),
    __("21 - Lockon + Any Direction + Melee"),
    __("22 - Lockon + Any Direction + Gun"),
    __("23 - Lockon + Any Direction + Style"),
    __("24 - Any Direction + Melee"), 
    __("25 - Any Direction + Gun"),
    __("26 - Any Direction + Style"),
    __("27 - Melee"),
    "28 -",
    "29 -",
    "30 -",
    "31 -",
    __("32 - Gun Release"),
    __("33 - Gun Charged Release"),
    __("34 - Lockon + Gun Charge"),
    __("35 - Gun Charged"),
    __("36 - Gun Charge3"),
    __("37 - Gun Charge4"),
    __("38 - Gun Charge3"),
    "39 -",
    "40 -",
    "41 -",
    "42 -",
    "43 -",
    "44 -",
    "45 -",
    "46 -",
    "47 -",
    "48 -",
    "49 -",
    "50 -",
    "51 -",
    "52 -",
    "53 -",
    "54 -",
    "55 -", 
    "56 -",
    "57 -",
    "58 -",
    "59 -",
    "60 -",
    "61 -",
    "62 -",
    "63 -",
    "64 -",
    "65 -",
    "66 -",
    "67 -",
    "68 -",
    "69 -",
    "70 -",
    "71 -",
    "72 -",
    "73 -",
    "74 -",
    "75 -",
    "76 -",
    "77 -",
    "78 -",
    "79 -",
    "80 -",
    __("81 - Melee + Style"),
    __("82 - Lockon + Backforward + Melee"),
    "83 -",
    __("84 - Gun")
};

static const char* attackNames[] = {
    "0 -",
};

static const char* weaponNames[] = {
    __("FF - None"),
    __("0 - Red Queen"),      // ID_RED_QUEEN = 0
    __("1 - Blue Rose"),      // ID_BLUE_ROSE = 1
    __("2 - Yamato"),         // ID_YAMATO = 2
    __("3 - Right Hand"),     // ID_RIGHT_HAND = 3
    __("4 - Rebellion"),      // ID_REVELLION = 4
    __("5 - Gilgamesh"),      // ID_GILGAMESH = 5
    __("6 - Lucifer"),        // ID_BASILISK = 6
    __("7 - Coyote Ace"),     // ID_COYOTE_ACE = 7
    __("8 - Pandora"),        // ID_PANDORA = 8
    __("9 - Ebony & Ivory"),  // ID_EBONY_IVORY = 9
    "10 -",
    "11 -",
    "12 -",
    "13 -",
    "14 -",
    "15 -",
    "16 -",
    "17 -",
    "18 -",
    "19 -",
    "20 -",
};

static const char* styleNames[] = {
    __("FF - None"),
    __("0 - Swordmaster"),
    __("1 - Gunslinger"),
    __("2 - Trickster"),
    __("3 - Royal Guard"),
    __("4 - Darkslayer"),
};

static const char* aerialNames[] = {
    __("0 - None"),
    __("1 - Ground"),
    __("2 - Air"),
    __("3 - Ground or air"),
};

static const char* restrictNames[] = {
    __("0 - None?"),
    __("1 - None?"),
    __("2 - Disabled?"),
    __("3 -"),
    __("4 - Height Restricted"),
};

static const char* dtNames[] = {
    __("0 - DT or no DT"),
    __("1 - DT or no DT 2"),
    __("2 - Disabled?"),
    __("3 - DT Only"),
    __("4 -"),
    __("5 -"),
    __("6 -"),
    __("7 -"),
    __("8 -"),
    __("9 - DT or Charge"),
    __("10 -"),
    __("11 - DT or Charge 2"),
    "12 -",
    "13 -",
    "14 -",
    "15 -",
    "16 -",
    "17 -",
    "18 -",
    "19 -",
    "20 -",
    "21 -",
    "22 -",
    "23 -",
    "24 -",
    "25 -",
    "26 -",
    "27 -",
    "28 -",
    "29 -",
    "30 -",
};

class cAreaJump;
class roomInfo;

class aGame
{
public:
	virtual void function0(); //0+0x00
	virtual void function1(); //0+0x04
	virtual void function2(); //0+0x08
	virtual void function3(); //0+0x0C
	virtual void function4(); //0+0x10
	virtual void function5(); //0+0x14
	virtual void function6(); //0+0x18
	virtual void function7(); //0+0x1C
	virtual void function8(); //0+0x20
	virtual void function9(); //0+0x24

    char pad_4[0x28];
    roomInfo* roomInfoPtr1; // 0x2c
    char pad_30[0x20];
    Vector3f unk_vec; // 0x50
    Vector3f unk_vec2; // 0x5c
    int init_jump; // 0x68
    int room_id; // 0x6c
    int door_entered; // 0x70
    int bp_floor; // 0x74
    char pad_78[0x159];
    bool m_paused; // 0x1d1
}; // Size: 0x1d2
static_assert(sizeof(aGame) == 0x1d2);

class currentMission
{
public:
    char pad_0[0x20];
    int restart; // 0x20
    char pad_24[0x4];
    int currentCharacter; // 0x28
    char pad_2c[0xdc];
    int roomNumber; // 0x108
    int roomNumber2; // 0x10c
}; // Size: 0x110
static_assert(sizeof(currentMission) == 0x110);

class pauseMenu {
public:
    char pad_0[0x14];
    uint8_t draw; // 0x14
    char pad_15[0x2b];
    bool optionHovered; // 0x40
}; // Size: 0x41
static_assert(sizeof(pauseMenu) == 0x41);

class currentRoom
{
public:
    char pad_0[0x20];
    bool allowPause; // 0x20
    char pad_21[0x10];
    bool isPaused; // 0x31
    char pad_32[0xa];
    uint32_t restartLastEvent; // 0x3c
    char pad_40[0xd4];
    pauseMenu* pauseMenuPtr1; // 0x114
}; // Size: 0x118
static_assert(sizeof(currentRoom) == 0x118);

class sArea
{
public:
    char pad_0x0000[0x3830]; // 0x0000
    aGame* aGamePtr; // 0x3830 aGame
    currentMission* currentMissionPtr; // 0x3834
    currentRoom* currentRoomPtr; // x03838
}; // Size: 0x383C
static_assert(sizeof(sArea) == 0x383C);

class sRender
{
public:
    char pad_0x0000[0x44]; // 0x00
    glm::ivec2 screenRes; // 0x44
};

class roomInfo
{
public:
	virtual void function0(); //0+0x00
	virtual void function1(); //0+0x04
	virtual void function2(); //0+0x08
	virtual void function3(); //0+0x0C
	virtual void function4(); //0+0x10
	virtual void function5(); //0+0x14
	virtual void function6(); //0+0x18
	virtual void function7(); //0+0x1C
	virtual void function8(); //0+0x20
	virtual void function9(); //0+0x24

	char n0000127_e[20]; //0x11E5888 
	char pad_0x0018[0x3EC]; //0x0018

}; //Size=0x0404
static_assert(sizeof(roomInfo) == 0x404);

class sWorkRate;
class unkClass;
class unkClass2;

class sWorkRate
{
public:
	virtual void function0(); //0+0x00
	virtual void function1(); //0+0x04
	virtual void function2(); //0+0x08
	virtual void function3(); //0+0x0C
	virtual void function4(); //0+0x10
	virtual void function5(); //0+0x14
	virtual void function6(); //0+0x18
	virtual void function7(); //0+0x1C
	virtual void function8(); //0+0x20
	virtual void function9(); //0+0x24

	char pad_0x0004[0x1C]; //0x0004
	float game_speed; //0x0020 
	float unk_float2; //0x0024 
	float global_speed; //0x0028 
	float player_speed; //0x002C 
	float enemy_speed; //0x0030 
	float room_speed; //0x0034 
	float turbo_speed; //0x0038 
	float unk_float3; //0x003C 
	char pad_0x0040[0x10]; //0x0040
	unkClass* unk_class_ptr; //0x0050 
	char pad_0x0054[0x4]; //0x0054

}; //Size=0x0058
static_assert(sizeof(sWorkRate) == 0x58);

class unkClass
{
public:
	unkClass2* next; //0x0000 
	uint32_t n0000045_c; //0x0004 
	char pad_0x0008[0x4]; //0x0008
	uint32_t n0000045_e; //0x000C 
	char pad_0x0010[0x10]; //0x0010
	uint32_t n00000463; //0x0020 
	char pad_0x0024[0x8]; //0x0024

}; //Size=0x002C
static_assert(sizeof(unkClass) == 0x2C);

class unkClass2
{
public:
	uint32_t next_ptr; //0x0000 
	char pad_0x0004[0x400]; //0x0004

}; //Size=0x0404
static_assert(sizeof(unkClass2) == 0x404);

class Inputs
{
public:
	virtual void function0(); //0+0x00
	virtual void function1(); //0+0x04
	virtual void function2(); //0+0x08
	virtual void function3(); //0+0x0C
	virtual void function4(); //0+0x10
	virtual void function5(); //0+0x14
	virtual void function6(); //0+0x18
	virtual void function7(); //0+0x1C
	virtual void function8(); //0+0x20
	virtual void function9(); //0+0x24

	__int32 button_pressed; //0x0004 
	__int32 button_released; //0x0008 
	__int32 button_went_down; //0x000C 
	__int32 button_pressed02; //0x0010 
	char pad_0x0014[0x4]; //0x0014
	float unk_float; //0x0018 
	char pad_0x001_c[0x18]; //0x001C

}; //Size=0x0034
static_assert(sizeof(Inputs) == 0x34);

class MtObject;

class MtDTI
{
public:
	char *m_name; //0x0004
	class MtDTI *mp_next; //0x0008
	class MtDTI *mp_child; //0x000C
	class MtDTI *mp_parent; //0x0010
	class MtDTI *mp_link; //0x0014
	size_t m_size; //0x0018
	uint32_t m_id; //0x001C
	// hope it's fucking thiscall
	//virtual void vecDelDtor(MtDTI* this, unsigned int x)();
	//virtual void MtObject* newInstance(MtDTI *this)();

	virtual void vec_del_dtor(unsigned int x) {};
	virtual void* new_instance() {return nullptr;};
}; //Size: 0x0020

class MtObject
{
public:
	virtual void vec_del_dtor(unsigned int i) {}; // 0x00
	virtual void create_ui(void* prop) {}; // 0x04
	virtual bool is_enable_instance() {return 1;}; // 0x08
    virtual void create_property(void* prop) {}; // 0x0C
    virtual MtDTI* get_dti() { return (MtDTI*)0x00E5C5A8; }; // 0x10
}; //Size: 0x0004
static_assert(sizeof(MtObject) == 0x04);

class CResource : public MtObject
{
public:
    virtual void callDestructor(); //0x0
    virtual void getMtFileInfo();//0x14
    virtual void getExtension(); //0x18

	char path[64]; //0x0004
	uint32_t m_ref_count; //0x0044
	uint32_t m_attr; //0x0048
	uint32_t m_flags_something; //0x004C
	uint32_t m_size; //0x0050
	int64_t m_id; //0x0054
	char pad_005_c[4]; //0x005C
}; //Size: 0x0060
static_assert(sizeof(CResource) == 0x60);

class REffectList : public CResource
{
public:
	float m_base_fps; //0x0060
	char *m_param_buf; //0x0064
	uint32_t m_param_buff_size; //0x0068
	uint32_t m_list_num; //0x006C
	uint32_t n00002_d88; //0x0070
	uint32_t n00002_d89; //0x0074
	uint32_t m_resource_ptr; //0x0078
	uint32_t m_resource_info_num; //0x007C
}; //Size: 0x0080
static_assert(sizeof(REffectList) == 0x0080);

//class uActor {
//    virtual void destructor();//0x00
//    virtual void getTypeInfo();//0x04
//    virtual void ukn1();//0x08
//    virtual void ukn2();//0x0C
//    virtual void getDTI();//0x10
//    virtual void setup();//0x14                                      
//    virtual void freeze(); //0x18, as in literally frosted over
//    virtual void ukn4(); //0x1C
//    virtual void ukn5(); //0x20
//    virtual void render(void* mtrans); // 0x24
//    virtual void ukn7(); // 0x28
//    virtual void ukn8(); // 0x2C
//    virtual void die(); // 0x30
//    virtual void updateLmat();//0x34
//    virtual void updateWmat();//0x38
//    virtual void getJointMatrix(int jntInd);//0x3C
//   // virtual void LoadrModel(CResource* mdl);//0x40
//    virtual void lifecycle();//0x78
//    virtual void getCenterPos();//0x84
//    virtual void getCenterJointInd(int index);//0x88
//    virtual void DamageReaction();//0x90
//    virtual void AtckHitCallback();//0x94
//
//    uintptr_t vtable;
//    int transFlags;
//};

//class MtMatrix
//{
//public:
//	Vector4f m1; //0x0000
//	Vector4f m2; //0x0010
//	Vector4f m3; //0x0020
//	Vector4f m4; //0x0030
//}; //Size: 0x0040
//static_assert(sizeof(MtMatrix) == 0x40);

class CUnit : public MtObject
{
public:
    union {
        uint32_t bitfield;
        struct {
        uint16_t pad0;
        uint8_t mTransMode;
        uint8_t mTransView;
        };
    };
	class CUnit *mp_next_unit; //0x0008
	class CUnit *mp_prev_unit; //0x000C
	float m_delta_time; //0x0010
	char reserved_state_flags[4]; //0x0014
}; //Size: 0x0018
static_assert(sizeof(CUnit) == 0x18);

//struct MtVector3 { /* 10 */
//    float x; // 0x00
//    float y; // 0x04
//    float z; // 0x08
//    int padding; // 0x0C
//};
//
//struct MtCapsule {
//    MtVector3 p0;
//    MtVector3 p1;
//    float r;
//    char field3_0x24;
//    char field4_0x25;
//    char field5_0x26;
//    char field6_0x27;
//    char field7_0x28;
//    char field8_0x29;
//    char field9_0x2a;
//    char field10_0x2b;
//    char field11_0x2c;
//    char field12_0x2d;
//    char field13_0x2e;
//    char field14_0x2f;
//};
//static_assert(sizeof(MtCapsule) == 0x30);

enum BE_FLAG {
    BEFLAG_DISABBLE,
    BEFLAG_PRE_MOVE,
    BEFLAG_MOVE,
    BEFLAG_PRE_DELETE,
    BEFLAG_DELETE
};

enum UNIT_ATTR {
    UATTR_MOVE = 1,
    UATTR_DRAW = 2,
    UATTR_SELECT = 4,
    UATTR_FIX = 8,
    UATTR_VISIBLE = 16
};

struct cUnit {
    void *vtable_ptr;
    union {
        uint32_t raw;
        struct {
            uint32_t mBeFlag : 3;
            uint32_t mMoveLine : 7;
            uint32_t mUnitAttr : 6;
            uint32_t mTransMode : 8;
            uint32_t mTransView : 8;
        } bits;
    } flags;
    cUnit *mp_next_unit;
    cUnit *mp_prev_unit;
    float m_delta_time;
    uint8_t reserved_state_flags[4];
};
static_assert(sizeof(cUnit) == 0x18);

class uCoord: public cUnit {
public:
    uCoord* mpParent;
    int ParentJoint;
    int mOrder;
    int padding24[3];
    MtVector3 mPos;
    MtVector4 mQuat;
    MtVector3 mScale;
    MtMatrix mLmat;
    MtMatrix mWmat;
};
static_assert(sizeof(uCoord) == 0xe0);

class UCoord : public CUnit
{
public:
	class UCoord *mp_parent; //0x0018
	uint32_t mParentNo;//attached entity's joint index
    uint32_t mOrder;//axis order
    char pad_024_c[0xC];
	Vector3f m_pos; //0x0030
	char pad_003_c[4]; //0x003C
	Vector4 m_quat; //0x0040
	Vector3f m_scale; //0x0050
    char pad_05C_c[4];
	Matrix4x4 m_lmat; //0x0060
	Matrix4x4 m_wmat; //0x00A0
}; //Size: 0x00E0
static_assert(sizeof(UCoord) == 0xE0);

class UFilter : public CUnit
{
public:
	uint16_t m_pass; //0x0018
	char pad_001_a[2]; //0x001A
	uint32_t m_priority; //0x001C
}; //Size: 0x0020
static_assert(sizeof(UFilter) == 0x20);

//class MtHermiteCurve
//{
//public:
//	float x[8]; //0x0000
//	float y[8]; //0x0020
//}; //Size: 0x0040
//static_assert(sizeof(MtHermiteCurve) == 0x40);

enum class CcType : int32_t
{
	TYPE_UNUSED = 0,
	TYPE_HUE = 1,
	TYPE_CONTRAST = 2,
	TYPE_CONTRAST2 = 3,
	TYPE_CHROMA = 4,
	TYPE_CHROMA2 = 5,
	TYPE_BRIGHTNESS = 6,
	TYPE_BRIGHTNESS2 = 7,
	TYPE_SCALE = 8,
	TYPE_SCALE2 = 9,
	TYPE_INPUTLEVEL = 10,
	TYPE_INPUTLEVEL2 = 11,
	TYPE_OUTPUTLEVEL = 12,
	TYPE_OUTPUTLEVEL2 = 13,
	TYPE_SEPIA = 14,
	TYPE_SEPIA2 = 15,
	TYPE_NEGA = 16,
	TYPE_NEGA2 = 17,
	TYPE_GRAYSCALE = 18,
	TYPE_RREPLACE = 19,
	TYPE_GREPLACE = 20,
	TYPE_BREPLACE = 21,
	TYPE_SEPIA3 = 22
}; //0x0008

class UColorCorrectFilterCorrector
{
public:
	virtual void function0(); //0+0x00
	virtual void function1(); //0+0x04
	virtual void function2(); //0+0x08
	virtual void function3(); //0+0x0C
	virtual void function4(); //0+0x10
	virtual void function5(); //0+0x14
	virtual void function6(); //0+0x18
	virtual void function7(); //0+0x1C
	virtual void function8(); //0+0x20
	virtual void function9(); //0+0x24

	bool m_enable; //0x0004
    char pad_05[3]; //0x0005
	/*uint16_t N00002DAF; //0x0005
	uint8_t N00002DB0; //0x0007*/
	CcType m_type; //0x0008
	Vector4 m_factor; //0x000C
	Vector4 m_factor2; //0x001C
	char pad_002_c[4]; //0x002C
}; //Size: 0x0030
static_assert(sizeof(UColorCorrectFilterCorrector) == 0x30);

class UColorCorrectFilter : public UFilter
{
public:

	class UColorCorrectFilterCorrector correctors[16]; //0x0020
	Matrix4x4 m_matrix; //0x0320
	class MtHermiteCurve m_tone_curve_r; //0x0360
	class MtHermiteCurve m_tone_curve_g; //0x03A0
	class MtHermiteCurve m_tone_curve_b; //0x03E0
	Vector4 m_gamma; //0x0420
	class cTransTexture *c_tans_texture; //0x0430
}; //Size: 0x0434
static_assert(sizeof(UColorCorrectFilter) == 0x434);

class UEnemySomething
{
public:
	char pad_0000[4992]; //0x0000
	Vector3f m_spawn_coords; //0x1380
	char pad_138_c[8]; //0x138C
	uint8_t n00004023; //0x1394
	char pad_1395[3]; //0x1395
	int32_t m_enemy_spawn_effect_something; //0x1398
	char pad_139_c[12]; //0x139C
	uint32_t unk_type_something; //0x13A8
	uint32_t n00003_fef; //0x13AC
	uint32_t n00003_f_f0; //0x13B0
	uint32_t n00003_f_f1; //0x13B4
	uint32_t n00003_f_f2; //0x13B8
	uint32_t n00003_f_f3; //0x13BC
	Matrix4x4 n00003_f_f4; //0x13C0
	uint32_t n00003_f_f5; //0x1400
	uint8_t n00003_f_f6; //0x1404
	uint8_t n00004033; //0x1405
	uint8_t n00004037; //0x1406
	char pad_1407[1]; //0x1407
	float n00003_f_f7; //0x1408
	uint32_t n00003_f_f8; //0x140C
	uint32_t n00003_f_f9; //0x1410
	uint32_t n00003_ffa; //0x1414
	uint32_t n0000403_d; //0x1418 Ptr
	char pad_141_c[24]; //0x141C
	float n00004000; //0x1434
	uint8_t n00004001; //0x1438
	char pad_1439[3]; //0x1439
	uint32_t n00004002; //0x143C
	uint32_t n00004003; //0x1440
	uint32_t n00004004; //0x1444
	char pad_1448[228]; //0x1448
	bool n00003_c7_b; //0x152C
	bool n00003_fdd; //0x152D
	bool n00003_f_e0; //0x152E
	char pad_152_f[4409]; //0x152F
}; //Size: 0x2668
static_assert(sizeof(UEnemySomething) == 0x2668);

class uEnemyDamagePossibility {
public:
    char pad_00[4]; // 0x0
}; // Size: 0x4

class uDamage {
public:
    char pad_0[0x18];
    float HP; // 0x18
    float HPMax; // 0x1c
    char pad_20[0x8];
    float prevDamageResist; // 0x28 // damage resist * difficulty
    float HPTaken; // 0x2c
    char pad_30[0xc];
    int stun[5]; // 0x3c
    int displacement[5]; // 0x50
    int unknown[8]; // 0x64
    char pad_84[0x8];
    bool isDead; // 0x8c
    char pad_8d[0x1f];
    float stunResetTimer; // 0xAC
}; // Size: 0xb0
static_assert(sizeof(uDamage) == 0xb0);

class lockOnSphereData {
public:
    Vector3f location; // 0x0
    float unknFloat1; // 0xc
    int unknInt1; // 0x10
    float unknFloat2; // 0x14
    float unknFloat3; // 0x18
    float unknFloat4; // 0x1c
    int unknInt2; // 0x20
    float unknFloat5; // 0x24
    float unknFloat6; // 0x28
    char pad_2C[4]; // 0x2c
}; // Size: 0x30
static_assert(sizeof(lockOnSphereData) == 0x30);



class uHasDelta : public cUnit {
public:
    char pad_18[4]; // 0x18
}; // Size: 0x1c

class uShadow : public cUnit {
public:
    private: char pad_18[0x28]; public:
    Vector3f rotation; // 0x40
    private: char pad_4c[0x4]; public:
    float howMuchToIgnoreModels; // 0x50
    private: char pad_54[0xc]; public:
    Vector4f colour; // 0x60
    private: char pad_70[0x30]; public:
    float diffuse; // 0xa0
    private: char pad_a4[0x10]; public:
    float solidity; // 0xb4
    private: char pad_b8[0x40]; public:
    float bloom; // 0xf8
    private: char pad_fc[0x128]; public:
    uint32_t resolution; // 0x224
}; // Size: 0x228
static_assert(sizeof(uShadow) == 0x228);

class SMediator {
public:
    char pad_0[0x24];
    class uPlayer* player_ptr; // 0x24
    char pad_28[0x88];
    class uEnemy* uBoss1; // 0xb0
    char pad_b4[0x1c];
    class uCameraCtrl* camera1; // 0xd0
    char pad_d4[0xc];
    uint32_t missionID; // 0xe0 // always shows 50 for BP
    uint32_t roomID; // 0xe4
    char pad_e8[0x2c];
    uint32_t orbCount; // 0x114
    uint32_t orbMissionCurrent; // 0x118
    uint32_t orbMissionPotential; // 0x11C
    char pad_118[0x14];
    float enemyDifficulty; // 0x134
    char pad_138[0x4];
    uint32_t hellGoldOrbCount; // 0x13c
    GameDifficulty gameDifficulty; // 0x140
    GameMode gameMode; // 0x144
    char pad_148[0x4];
    float stylePoints; // 0x14C
    char pad_150[0x68];
    class uEnemy* uEnemies[10]; // 0x1b8
    uint32_t enemyCount[3]; // 0x1e0
    char pad_1ec[0x64];
    float bpTimer; // 0x250
    char pad_254[0x114];
    bool hasUnlockedDT; // 0x368
    bool hasUnlockedAirHike; // 0x369
    char pad_36a[0x1];
    bool hasUnlockedSprint; // 0x36b
    char pad_36c[0x36];
    bool hasUnlockedHold; // 0x3a2
    bool hasUnlockedMaxAct; // 0x3a3
    char pad_3a4[0x5];
    bool displayDisasterGauge; // 0x3a9
    char pad_3aa[0xea];
    bool turboEnabled; // 0x494
    char pad_495[0x38b];
    sWorkRate workRate; // 0x820
}; // Size: 0x878
static_assert(sizeof(SMediator) == 0x878);

class MotionData {
public:
    char pad_0[0x4];
    uint32_t hitboxStartTime; // 0x4
    char pad_8[0xc];
    uint32_t passThroughEnemiesIdk; // 0x14
    char pad_18[0x24];
    uint32_t sfxTiming; // 0x3c
    char pad_40[0x10];
    uint32_t sfxToPlay; // 0x50
}; // Size: 0x54
static_assert(sizeof(MotionData) == 0x54);

class InnerMotionPtr {
public:
    MotionData motionData[100]; // 0x0
}; // Size: 0x20d0
static_assert(sizeof(InnerMotionPtr) == 0x20d0);

class MotionPtr {
public:
    char pad_0[0x4];
    char motionString1[30]; // 0x4
    char pad_22[0x17e];
    class InnerMotionPtr* innerMotionPtr1; // 0x1a0
}; // Size: 0x1a4
static_assert(sizeof(MotionPtr) == 0x1a4);

class LuciferPin {
public:
    uint8_t pad_0[0x14];
    uint8_t pin_state; //0x14 (3-pinned, 4-primed, 7-climax)
    uint8_t pop_state; //0x15
    char pad_15[2]; //0x16
    uintptr_t PinnedTarget; //0x18
    char pad_1C[0x14]; //0x1C
    Vector3f position; //0x30
    char pad_3C[4]; //0x3C
    Vector4 rotation; //0x40
    Vector3f scale; //0x50
    char pad_5c[0x1734]; //0x5C
    float timer; // 0x1790
    float timerMax; // 0x1794
    char pad_1798[0x1c];
    bool penetrated; // 0x17b4
}; // Size: 0x17b5
static_assert(sizeof(LuciferPin) == 0x17b5);

class DevilArm {
public:
    char pad_0[0x1370];
    uint8_t ID; // 0x1370
    char pad_1374[0xb];
    bool visible; // 0x137c
    char pad_137d[0x47];
    float roseYAxisVelocity; // 0x13c4
    char pad_13c8[0x20];
    bool roseAlive; // 0x13e8
}; // Size: 0x13e9
static_assert(sizeof(DevilArm) == 0x13e9);

class CharacterSettingsOne {
public:
    char pad_0[0x1c];
    bool groundedActual; // 0x1c
    char pad_1d[0x143];
    float offsetFromGround; // 0x160
}; // Size: 0x164
static_assert(sizeof(CharacterSettingsOne) == 0x164);

class CharacterSettingsTwo {
public:
    char pad_0[0xC8];
    float sprintTimer; // 0xc8
    float sprintSpeedMultiplier; // 0xd0
    char pad_d4[0xa4];
    float fallSpeed; // 0x174
}; // Size: 0x178
static_assert(sizeof(CharacterSettingsTwo) == 0x178);

class CharacterSettingsThree {
public:
    char pad_0[0x8];
    float stingerSpeed; // 0x8
    float stingerLength; // 0xc
    char pad_10[0x14];
    float firstShotEbonyIvoryBounce; // 0x24
    char pad_28[0x40];
    float rainstormBounce; // 0x68
    char pad_6c[0xc8];
    float trickTravelSpeed; // 0x134
    float trickTravelDistance; // 0x138
    char pad_13c[0x18];
    float rainstormHeightRestriction; // 0x154
    char pad_158[0x20];
    float crossunderBanDistance; // 0x178
    char pad_17c[0x8c];
    float jealousyBounce; // 0x208
    char pad_20c[0x8];
    float snatchLengthLv1; // 0x214
    float snatchLengthLv2; // 0x218
    float snatchLengthLv3; // 0x21c
    char pad_220[0xc];
    Vector3f snatchEnemyOffset; // 0x22c
    char pad_238[0x164];
    float funshipDrainRate; // 0x39c
    char pad_3a0[0x40];
    float funshipBounce; // 0x3e0
    float funshipUnbounce; // 0x3e4
}; // Size: 0x3e8
static_assert(sizeof(CharacterSettingsThree) == 0x3e8);

struct kAtckDefTbl {
    uint32_t atckAttr;  //0x0000
    uint32_t atckId;    //0x0004
    uint32_t atckLevel; //0x0008
    uint32_t atckInfo;  //0x000C
    union {
        uint32_t buffer; //0x0010
        struct {
            uint8_t atckCommand;   //0x0010
            uint8_t atckCommandNo; //0x0011
            uint8_t atckCondition; //0x0012
            uint8_t ukn;           //0x0013
        };
    } command;
    uint32_t atckConditionWp;    //0x0014
    uint32_t atckConditionStyle; //0x0018
    uint32_t ukn;                //0x001C (aerial lock)
    uint32_t atckAs;             //0x0020
    uint32_t cancelId[5];        //0x0024, 0x0028, 0x002C, 0x0030, 0x0034
}; //Size: 0x38
static_assert(sizeof(kAtckDefTbl) == 0x38);

class UStageSetTimeSlow
{
public:
	char pad_0000[4992]; //0x0000
	float m_time_left; //0x1380
	float m_duration; //0x1384
	float m_enemy_work_rate; //0x1388
	float m_stage_work_rate; //0x138C
	char pad_1390[3288]; //0x1390
}; //Size: 0x2068
static_assert(sizeof(UStageSetTimeSlow) == 0x2068);

class N00002D7A
{
public:
	char pad_0000[4]; //0x0000
}; //Size: 0x0004
static_assert(sizeof(N00002D7A) == 0x4);

class MtAllocator : public MtObject
{
public:
	uint32_t m_used_size; //0x0004
	uint32_t m_max_used_size; //0x0008
	uint32_t m_total_size; //0x000C
	char *m_name; //0x0010
	uint16_t m_type; //0x0014
	uint16_t m_attr; //0x0016
	uint32_t m_owner; //0x0018
	class MtCriticalSection *m_cs; //0x001C
    virtual MtObject* allocate(size_t size, int align_mask);
    virtual uint16_t deallocate(MtObject* object); // 0x0018

}; //Size: 0x0020
static_assert(sizeof(MtAllocator) == 0x0020);

class MtHeapAllocator : public MtAllocator
{
public:
	char pad_0020[60]; //0x0020
}; //Size: 0x005C
static_assert(sizeof(MtHeapAllocator) == 0x005C);

class UEfctCam
{
public:
	char pad_0000[512]; //0x0000
	uint32_t some_field; //0x0200
}; //Size: 0x0204
static_assert(sizeof(UEfctCam) == 0x0204);

class UTvNoiseFilter : public UFilter
{
public:
	Vector3f m_noisy_uv_offset; //0x0020
	char pad_002_c[8]; //0x002C
	float m_noise_power; //0x0034
	float m_noise_power_croma; //0x0038
	float m_scale_y; //0x003C
	float m_scale_cr_cb; //0x0040
	uint32_t m_scanline_size; //0x0044
	float m_scanline_alpha; //0x0048
	uint32_t noise_texture_ptr; //0x004C
	uint32_t mp_tv_mask_texture_ptr; //0x0050
	bool m_enable_blank_scan; //0x0054
	char pad_0055[3]; //0x0055
	float m_blank_ofs_y; //0x0058
	float m_blank_speed; //0x005C
	uint8_t m_blank_alpha; //0x0060
	char pad_0061[3]; //0x0061
	float m_blank_size; //0x0064
	float m_shock_noise_freq; //0x0068
	float m_h_sync_noise_amplitude; //0x006C
	float m_v_sync_noise_amplitude; //0x0070
	float m_shock_detail_reduction; //0x0074
	float m_h_sync_blank_width; //0x0078
	float m_v_sync_blank_width; //0x007C
	Vector4 m_v_sync_blank_color; //0x0080
	uint32_t m_shock_noise_time; //0x0090
	float m_h_sync_noise_value; //0x0094
	float m_v_sync_noise_value; //0x0098
	float m_h_sync_lag; //0x009C
	float m_v_sync_lag; //0x00A0
	float m_detail_reduction; //0x00A4
	float m_h_sync_noise_offset; //0x00A8
	char pad_00_ac[1992]; //0x00AC
}; //Size: 0x0874
static_assert(sizeof(UTvNoiseFilter) == 0x0874);

class SStylishCount
{
public:
	char pad_0000[32]; //0x0000
	uint32_t current_style_tier; //0x0020
	char pad_0024[508]; //0x0024
}; //Size: 0x0220
static_assert(sizeof(SStylishCount) == 0x220);

class UEm003Shl : public MtObject
{
public:
	char pad_0004[44]; //0x0004
	Vector3f m_pos_maybe; //0x0030
	char pad_003_c[3716]; //0x003C
	Vector3f m_some_float_vec; //0x0EC0
	char pad_0_ecc[2260]; //0x0ECC
	uint32_t m_required_idk_wtf_is_this_needs_to_be2; //0x17A0
	uint32_t m_required_idk_wtf_is_this_needs_to_be6; //0x17A4
	char pad_17_a8[12]; //0x17A8
	uint16_t n00005_a23; //0x17B4
	uint8_t m_required_some_byte_idk; //0x17B6
	char pad_17_b7[57]; //0x17B7
}; //Size: 0x17F0
static_assert(sizeof(UEm003Shl) == 0x17F0);

/* 8554 */
struct __declspec(align(4)) MtStringSTRING
{
    int ref;
    unsigned int length;
    char str[1];
};

/* 8555 */
struct MtString
{
    MtStringSTRING *value;
};

class MtCriticalSection
{
public:
    int8_t cs[28]; //0x0000
}; //Size: 0x001C
static_assert(sizeof(MtCriticalSection) == 0x1C);

class CSystem : public MtObject
{
public:
    class MtCriticalSection critical_section; //0x0004
}; //Size: 0x0020
static_assert(sizeof(CSystem) == 0x0020);

class SRender : public CSystem
{
public:
    char pad_0020[20]; //0x0020
    IDirect3DDevice9* mp_device; //0x0034
    char pad_0038[40]; //0x0038
}; //Size: 0x0060
static_assert(sizeof(SRender) == 0x0060);

class SDevil4Main
{
public:
    char pad_0000[66376]; //0x0000
    class SRender *s_render; //0x10348
    char pad_1034_c[60]; //0x1034C
}; //Size: 0x10388
static_assert(sizeof(SDevil4Main) == 0x10388);

class SDevil4MainPtr
{
public:
    class SDevil4Main *mp_devil4_main; //0x0000
    char pad_0004[60]; //0x0004
}; //Size: 0x0040
static_assert(sizeof(SDevil4MainPtr) == 0x40);

class sMouse
{
public:
    char pad_0000[144]; //0x0000
    bool m_show_mouse_cursor; //0x0090
    bool N00000272; //0x0091
    bool N00000276; //0x0092
    char pad_0093[2029]; //0x0093
}; //Size: 0x0880
static_assert(sizeof(sMouse) == 0x880);

class sMousePtr
{
public:
    class sMouse *m_mouse_ptr; //0x0000
}; //Size: 0x0004
static_assert(sizeof(sMousePtr) == 0x4);

class MoveLine {
public:
    void* vtable;  // 0x0
    char* mName; // 0x4
    uint32_t mParallel : 1; // 0x8
    uint32_t mPause : 1;
    uint32_t mTrans : 1;
    uint32_t mLineType : 6;
    uint32_t reserved : 23;
    cUnit* mTop; // 0xC
    cUnit* mBottom; // 0x10
    float mDeltaTime; // 0x14
};
static_assert(sizeof(MoveLine) == 0x18);

class sUnit : public CSystem {
public:
    MoveLine mMoveLine[32];
};
static_assert(sizeof(sUnit) == 0x320);