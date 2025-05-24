#include "PlayerTracker.hpp"
#include "WorkRate.hpp"
#include "../sdk/Devil4.hpp"

#if 1
uintptr_t PlayerTracker::jmp_return = NULL;
uPlayer* PlayerTracker::player_ptr = NULL;

Vector3f PlayerTracker::savedPlayerPosition{ 0.0f, 0.0f, 0.0f };
float PlayerTracker::savedPlayerRotation = 0.0f;
int8_t PlayerTracker::savedPlayerWeight = 0;
float PlayerTracker::savedPlayerInertia = 0;
int8_t PlayerTracker::savedPlayerMoveID   = 0;
int8_t PlayerTracker::savedPlayerMoveBank = 0;
uint8_t PlayerTracker::savedPlayerCancels1[]{ 0, 0, 0, 0 };
uint32_t PlayerTracker::savedPlayerCancels2[]{ 0, 0, 0, 0 };
uint32_t PlayerTracker::savedPlayerSword = 0;
uint32_t PlayerTracker::savedPlayerGun = 0;
uint32_t PlayerTracker::savedPlayerStyle = 0;
uint32_t PlayerTracker::savedPlayerLockonAnimation = 0;
uint32_t PlayerTracker::savedPlayerCanWeaponChange = 0;
bool PlayerTracker::show_player_params = false;

struct ParamInfo {
    const char* name;
    int offset;
};

static const ParamInfo neroParams[] = {
	{"AIR_ATCK_COMMON_ADD_Y_SPD", 0x0},
	{"AIR_FLIPPED_Y_SPD", 0x1},
	{"AIR_FLIPPED_Y_ADD", 0x2},
	{"AIR_FLIPPED_XZ_SPD", 0x3},
	{"AIR_FLIPPED_XZ_ADD", 0x4},
	{"DT_GAUGE_DOWN", 0x5},
	{"DT_GAUGE_DOWN_POWER_UP", 0x6},
	{"DT_HP_RECOVER", 0x7},
	{"DT_GAUGE_KEEP_FRM", 0x8},
	{"DT_BUSTER_RATE", 0x9},
	{"DROPKICK_SPD_Z", 0xa},
	{"DROPKICK_ACC_Y", 0xb},
	{"AIR_DEVIL_TRG_SPD_Y", 0xc},
	{"AIR_DEVIL_TRG_ACC_Y", 0xd},
	{"RH_LONG_GRAB_STYLISH_POINT", 0xe},
	{"RH_SHORT_GRAB_STYLISH_POINT", 0xf},
	{"EG_GUARD_STYLISH_PTS", 0x10},
	{"EG_GUARD_STYLISH_TIMER", 0x11},
	{"EG_GUARD_DT_ADD", 0x12},
	{"RQ_AIR_COMBO0_SPD_XZ", 0x13},
	{"RQ_AIR_COMBO0_SPD_Y", 0x14},
	{"RQ_AIR_COMBO0_ADD_Y", 0x15},
	{"RQ_AIR_COMBO1_SPD_XZ", 0x16},
	{"RQ_AIR_COMBO1_SPD_Y", 0x17},
	{"RQ_AIR_COMBO1_ADD_Y", 0x18},
	{"RQ_AIR_COMBO2_SPD_XZ", 0x19},
	{"RQ_AIR_COMBO2_SPD_Y", 0x1a},
	{"RQ_AIR_COMBO2_ADD_Y", 0x1b},
	{"AIR_COMBO_HEIGHT_LIMIT", 0x1c},
	{"RQ_AIR_DELAY_SPD_Y", 0x1d},
	{"RQ_AIR_DELAY_ADD_Y", 0x1e},
	{"RQ_AIR_DELAY_HEIGHT_LIMIT", 0x1f},
	{"RQ_COMBO_B_SPD1", 0x20},
	{"RQ_COMBO_B_SPD2", 0x21},
	{"RQ_COMBO_B_SPD3", 0x22},
	{"RQ_COMBO_B_SPD4", 0x23},
	{"RQ_COMBO_B_SPD5", 0x24},
	{"RQ_COMBO_B_SPD6", 0x25},
	{"HITIME_JUMP_SPD", 0x26},
	{"HITIME_JUMP_ACC", 0x27},
	{"HITIME_MOVE_EM_DIST", 0x28},
	{"ATCK_STINGER_LV1_TIME", 0x29},
	{"ATCK_STINGER_LV1_RANGE", 0x2a},
	{"ATCK_STINGER_LV2_TIME", 0x2b},
	{"ATCK_STINGER_LV2_RANGE", 0x2c},
	{"ATCK_STINGER_MOT_MOVE_LEN", 0x2d},
	{"ATCK_STINGER_JUMP_XZ_SPD", 0x2e},
	{"ATCK_STINGER_JUMP_Y_SPD", 0x2f},
	{"ATCK_STINGER_JUMP_Y_ADD", 0x30},
	{"KABUTO_HEIGHT_LIMIT", 0x31},
	{"OD_HITIME_JUMP1_SPD", 0x32},
	{"OD_HITIME_JUMP1_ACC", 0x33},
	{"OD_HITIME_JUMP2_SPD", 0x34},
	{"OD_HITIME_JUMP2_ACC", 0x35},
	{"OD_HITIME_JUMP3_SPD", 0x36},
	{"OD_HITIME_JUMP3_ACC", 0x37},
	{"RQ_OD_HITIME_HITSLOW_RATE", 0x38},
	{"RQ_OD_HITIME_HITSLOW_TIME", 0x39},
	{"RQ_OD_STINGER_CHARGE_TIME", 0x3a},
	{"RQ_OD_STINGER_LV1_S_TIME", 0x3b},
	{"RQ_OD_STINGER_LV1_S_LEN", 0x3c},
	{"RQ_OD_STINGER_LV1_L_TIME", 0x3d},
	{"RQ_OD_STINGER_LV1_L_LEN", 0x3e},
	{"RQ_OD_STINGER_LV2_S_TIME", 0x3f},
	{"RQ_OD_STINGER_LV2_S_LEN", 0x40},
	{"RQ_OD_STINGER_LV2_L_TIME", 0x41},
	{"RQ_OD_STINGER_LV2_L_LEN", 0x42},
	{"RQ_OD_STINGER_LV3_S_TIME", 0x43},
	{"RQ_OD_STINGER_LV3_S_LEN", 0x44},
	{"RQ_OD_STINGER_LV3_L_TIME", 0x45},
	{"RQ_OD_STINGER_LV3_L_LEN", 0x46},
	{"RQ_OD_STINGER_SPD_DOWN_RATE", 0x47},
	{"DEMO_STINGER_TIME", 0x48},
	{"DEMO_STINGER_RANGE", 0x49},
	{"RQ_AIR_STINGER_TIME", 0x4a},
	{"RQ_AIR_STINGER_RANGE", 0x4b},
	{"RQ_AIR_STINGER_HEIGHT_LIMIT", 0x4c},
	{"RQ_AIR_STINGER_XZ_SPD_DECELERATE", 0x4d},
	{"RQ_FLIP_S_BACK_Z_SPD", 0x4e},
	{"RQ_FLIP_S_BACK_Z_ACC", 0x4f},
	{"RQ_FLIP_L_BACK_Z_SPD", 0x50},
	{"RQ_FLIP_L_BACK_Z_ACC", 0x51},
	{"OD_GUAGE_SPD", 0x52},
	{"OD_GUAGE_ADD", 0x53},
	{"OD_GUAGE_SPD_MAX", 0x54},
	{"OD_GUAGE_SUB", 0x55},
	{"OD_GUAGE_MAX", 0x56},
	{"OD_FOLLOW_THROUGH_REFUSE_FRM", 0x57},
	{"OD_AIR_HEIGHT_LIMIT", 0x58},
	{"OD_AIR_SPD_Y", 0x59},
	{"OD_AIR_ADD_Y", 0x5a},
	{"OD_AIR_SPD_Y_ADD", 0x5b},
	{"ATCK_AIR_GUN_XZ_SPD", 0x5c},
	{"ATCK_AIR_GUN_Y_SPD", 0x5d},
	{"ATCK_AIR_GUN_Y_ADD", 0x5e},
	{"ATCK_AIR_GUN_2ND_Y_SPD", 0x5f},
	{"ATCK_AIR_GUN_2ND_Y_ADD", 0x60},
	{"ATCK_AIR_GUN_XZ_SPD_MAX", 0x61},
	{"BR_LOCK_HOLD_TIME", 0x62},
	{"BR_AIR_XZ_SPD_RATE", 0x63},
	{"WP000_SHELL_LV2_LEN", 0x64},
	{"WP000_SHELL_LV3_LEN", 0x65},
	{"BR_AIR_CSHOT1_SPD_XZ", 0x66},
	{"BR_AIR_CSHOT1_RATE_XZ", 0x67},
	{"BR_AIR_CSHOT1_SPD_Y", 0x68},
	{"BR_AIR_CSHOT1_ADD_Y", 0x69},
	{"BR_AIR_CSHOT2_SPD_XZ", 0x6a},
	{"BR_AIR_CSHOT2_RATE_XZ", 0x6b},
	{"BR_AIR_CSHOT2_SPD_Y", 0x6c},
	{"BR_AIR_CSHOT2_ADD_Y", 0x6d},
	{"WP000_SHELL_SPEED", 0x6e},
	{"WP000_SHELL_LEN", 0x6f},
	{"WP000_SHELL_LEN_BOSS", 0x70},
	{"DEVIL_DRIVE_SHELL00_SPEED", 0x71},
	{"DEVIL_DRIVE_SHELL00_LEN", 0x72},
	{"DEVIL_DRIVE_SHELL00_LEN_BOSS", 0x73},
	{"DEVIL_DRIVE_SHELL_SPEED", 0x74},
	{"DEVIL_DRIVE_SHELL_LEN", 0x75},
	{"DEVIL_DRIVE_SHELL_LEN_BOSS", 0x76},
	{"DEVIL_DRIVE_SHELL_OFF_TIME", 0x77},
	{"DEVIL_DRIVE_SHELL_ADD_SPEED", 0x78},
	{"DEVIL_DRIVE_SHELL_MAX_SPEED", 0x79},
	{"DEVIL_DRIVE_SHELL_HIT_NUM00", 0x7a},
	{"DEVIL_DRIVE_SHELL_HIT_NUM01", 0x7b},
	{"DEVIL_DRIVE_SHELL_SHOT_TIME", 0x7c},
	{"ATCK_IS_HOLD_TIME", 0x7d},
	{"ATCK_IS_CSHOT_WAITTIME", 0x7e},
	{"ATCK_IS_SHOT_SPD", 0x7f},
	{"ATCK_IS_SHOT_ADD", 0x80},
	{"ATCK_IS_SHOT_ADD_MAX", 0x81},
	{"ATCK_IS_CSHOT_TIME", 0x82},
	{"ATCK_IS_ENABLE_TIME", 0x83},
	{"ATCK_IS_ENABLE_TIME_BOSS", 0x84},
	{"RH_LONG_GRAB_DIST_LV1", 0x85},
	{"RH_LONG_GRAB_DIST_LV2", 0x86},
	{"RH_LONG_GRAB_DIST_LV3", 0x87},
	{"AIR_LONG_GRAB_HEIGHT_LIMIT", 0x88},
	{"RH_LONG_AIR_GRAB_Y_SPD", 0x89},
	{"RH_LONG_AIR_GRAB_Y_ADD", 0x8a},
	{"RH_LONG_EM_DRAW_POS_X", 0x8b},
	{"RH_LONG_EM_DRAW_POS_Y", 0x8c},
	{"RH_LONG_EM_DRAW_POS_Z", 0x8d},
	{"RH_LONG_EM_SHORT_DRAW_POS_X", 0x8e},
	{"RH_LONG_EM_SHORT_DRAW_POS_Y", 0x8f},
	{"RH_LONG_EM_SHORT_DRAW_POS_Z", 0x90},
	{"RH_LONG_APPROACH_XZ_SPD_MAX", 0x91},
	{"RH_LONG_APPROACH_XZ_SPD_MIN", 0x92},
	{"RH_LONG_APPROACH_XZ_SPD_DECELERATE", 0x93},
	{"DEMO_RH_LONG_APPROACH_XZ_SPD_MAX", 0x94},
	{"DEMO_RH_LONG_APPROACH_XZ_SPD_MIN", 0x95},
	{"DEMO_RH_LONG_APPROACH_XZ_LEN_MAX", 0x96},
	{"DEMO_RH_LONG_APPROACH_XZ_LEN_MIN", 0x97},
	{"DEMO_RH_LONG_APPROACH_XZ_SPD_DECELERATE", 0x98},
	{"LG_AIR_FLIPPED_XZ_SPD", 0x99},
	{"LG_AIR_FLIPPED_XZ_RATE", 0x9a},
	{"LG_AIR_FLIPPED_Y_SPD", 0x9b},
	{"LG_AIR_FLIPPED_Y_ADD", 0x9c},
	{"AIR_SHORT_GRAB_HEIGHT_LIMIT", 0x9d},
	{"RH_SHORT_AIR_GRAB_Y_SPD", 0x9e},
	{"RH_SHORT_AIR_GRAB_Y_ADD", 0x9f},
	{"RH_SG_ANG_ADJUST_DIST", 0xa0},
	{"RH_SG_ANG_ADJUST_HEIGHT", 0xa1},
	{"RH_SG_ANG_ADJUST_BOTTOM", 0xa2},
	{"RH_SG_ANG_ADJUST_ANG", 0xa3},
	{"RH_SG_ANG_ADJUST_AT_DIST", 0xa4},
	{"RH_SG_ANG_ADJUST_AT_HEIGHT", 0xa5},
	{"RH_SG_ANG_ADJUST_AT_BOTTOM", 0xa6},
	{"RH_SHORT_AIR_GRAB_HIT_Y_SPD", 0xa7},
	{"RH_SHORT_AIR_GRAB_HIT_Y_ADD", 0xa8},
	{"RH_SHORT_AIR_GRAB_THROW_Y_SPD", 0xa9},
	{"RH_SHORT_AIR_GRAB_THROW_Y_ADD", 0xaa},
	{"RH_SHORT_AIR_SCAREBALL_HIT_SPD_Y", 0xab},
	{"RH_SHORT_AIR_SCAREBALL_HIT_ACC_Y", 0xac},
	{"RH_SHORT_AIR_SCAREBALL_ROLL_SPD_Y", 0xad},
	{"RH_SHORT_AIR_SCAREBALL_ROLL_ACC_Y", 0xae},
	{"RH_SHORT_AIR_SCAREBALL_END_SPD_Y", 0xaf},
	{"RH_SHORT_AIR_SCAREBALL_END_ACC_Y", 0xb0},
	{"RH_SHORT_AIR_BIANCO_HIT_SPD_Y", 0xb1},
	{"RH_SHORT_AIR_BIANCO_HIT_ACC_Y", 0xb2},
	{"RH_SHORT_AIR_BIANCO_STICK_SPD_Y", 0xb3},
	{"RH_SHORT_AIR_BIANCO_STICK_ACC_Y", 0xb4},
	{"RH_SHORT_AIR_BIANCO_BURST_SPD_Y", 0xb5},
	{"RH_SHORT_AIR_BIANCO_BURST_ACC_Y", 0xb6},
	{"RH_SHORT_BIANCO_MOT_SPD", 0xb7},
	{"RH_SHORT_BIANCO_MOT_SPD2", 0xb8},
	{"RH_SGD_BIANCO_MV_FRM_1", 0xb9},
	{"RH_SGD_BIANCO_MV_DIST_1", 0xba},
	{"RH_SGD_BIANCO_MV_FRM_2", 0xbb},
	{"RH_SGD_BIANCO_MV_DIST_2", 0xbc},
	{"RH_SGD_BIANCO_MV_FRM_3", 0xbd},
	{"RH_SGD_BIANCO_MV_DIST_3", 0xbe},
	{"RH_SGAD_BIANCO_DOWN_SPD_Y", 0xbf},
	{"RH_SHORT_AIR_BIANCO_PIERCE_SPD_Y", 0xc0},
	{"RH_SHORT_AIR_BIANCO_PIERCE_ACC_Y", 0xc1},
	{"RH_SGD_BIANCO_MAX_ANG", 0xc2},
	{"RH_SGD_BIANCO_BURST_SPD_Y", 0xc3},
	{"RH_SGD_BIANCO_BURST_ACC_Y", 0xc4},
	{"RH_SG_BIANCO_BURST_SPD_XZ", 0xc5},
	{"RH_SGD_BIANCO_BURST_SPD_XZ", 0xc6},
	{"RH_SHORT_AIR_ALTO_HIT_SPD_Y", 0xc7},
	{"RH_SHORT_AIR_ALTO_HIT_ACC_Y", 0xc8},
	{"RH_SHORT_AIR_ALTO_LIFT_SPD_Y", 0xc9},
	{"RH_SHORT_AIR_ALTO_LIFT_ACC_Y", 0xca},
	{"RH_SHORT_AIR_ALTO_DOWN_SPD_Y", 0xcb},
	{"RH_SHORT_AIR_ALTO_DOWN_ACC_Y", 0xcc},
	{"RH_SHORT_AIR_ALTO_D_DOWN_SPD_Y", 0xcd},
	{"RH_SHORT_AIR_ALTO_D_DOWN_ACC_Y", 0xce},
	{"RH_SGA_ALTO_HIT_Z_SPD", 0xcf},
	{"RH_SGA_D_ALTO_DOWN_Z_SPD", 0xd0},
	{"RH_SG_ALTO_ROTATE_FRM", 0xd1},
	{"RH_SHORT_AIR_FROST_SPD_Y", 0xd2},
	{"RH_SHORT_AIR_FROST_ACC_Y", 0xd3},
	{"RH_SHORT_AIR_FROST_SPD_Y_LP", 0xd4},
	{"RH_SHORT_AIR_FROST_ACC_Y_LP", 0xd5},
	{"RH_SHORT_AIR_FROST_SPD_Y_LP2", 0xd6},
	{"RH_SHORT_AIR_FROST_ACC_Y_LP2", 0xd7},
	{"RH_SHORT_AIR_FROST_SPD_Y_END", 0xd8},
	{"RH_SHORT_AIR_FROST_ACC_Y_END", 0xd9},
	{"RH_SHORT_AIR_FROST_MOT_SPD", 0xda},
	{"RH_SHORT_AIR_FROST_MOT_SPD2", 0xdb},
	{"RH_SHORT_AIR_FROST_D_MOT_SPD", 0xdc},
	{"RH_SHORT_AIR_FROST_D_MOT_SPD2", 0xdd},
	{"RH_SHORT_AIR_ASSAULT_HIT_SPD_Y", 0xde},
	{"RH_SHORT_AIR_ASSAULT_HIT_ACC_Y", 0xdf},
	{"RH_SHORT_AIR_ASSAULT_DOWN_SPD_Y", 0xe0},
	{"RH_SHORT_AIR_ASSAULT_DOWN_ACC_Y", 0xe1},
	{"RH_SHORT_AIR_ASSAULT_MOT_SPD", 0xe2},
	{"RH_SHORT_AIR_ASSAULT_MOT_SPD2", 0xe3},
	{"RH_SHORT_ASSAULT_COUNTER_DOWN_SPD_Y", 0xe4},
	{"RH_SHORT_ASSAULT_COUNTER_DOWN_ACC_Y", 0xe5},
	{"RH_SHORT_ASSAULT_COUNTER_DOWN_SPD_XZ", 0xe6},
	{"RH_SHORT_ASSAULT_MOT_SPD", 0xe7},
	{"RH_SHORT_AIR_BLITZ_HIT_SPD_Y", 0xe8},
	{"RH_SHORT_AIR_BLITZ_HIT_ACC_Y", 0xe9},
	{"RH_SHORT_AIR_BLITZ_LP_SPD_Y", 0xea},
	{"RH_SHORT_AIR_BLITZ_LP_ACC_Y", 0xeb},
	{"RH_SHORT_AIR_BLITZ_END1_SPD_Y", 0xec},
	{"RH_SHORT_AIR_BLITZ_END1_ACC_Y", 0xed},
	{"RH_SHORT_AIR_BLITZ_END2_SPD_Y", 0xee},
	{"RH_SHORT_AIR_BLITZ_END2_ACC_Y", 0xef},
	{"RH_SHORT_BLITZ_D_MOT_SPD", 0xf0},
	{"RH_SGA_CHIMAIRASEED_HIT_SPD_Y", 0xf1},
	{"RH_SGA_CHIMAIRASEED_HIT_ACC_Y", 0xf2},
	{"RH_SGA_CHIMAIRASEED_PULL_SPD_Y", 0xf3},
	{"RH_SGA_CHIMAIRASEED_PULL_ACC_Y", 0xf4},
	{"RH_SG_CHIMAIRASEED_MOT_SPD", 0xf5},
	{"RH_SHORT_CUTLASS_DOWN_SPD_Y", 0xf6},
	{"RH_SHORT_CUTLASS_DOWN_ACC_Y", 0xf7},
	{"RH_SHORT_CUTLASS_DOWN_SPD_XZ", 0xf8},
	{"RH_SHORT_AIR_CUTLASS_HIT_SPD_Y", 0xf9},
	{"RH_SHORT_AIR_CUTLASS_HIT_ACC_Y", 0xfa},
	{"RH_SHORT_CUTLASS_END_SPD_Y", 0xfb},
	{"RH_SHORT_CUTLASS_END_ACC_Y", 0xfc},
	{"RH_SHORT_CUTLASS_END_SPD_XZ", 0xfd},
	{"RH_SHORT_D_CUTLASS_END_SPD_Y", 0xfe},
	{"RH_SHORT_D_CUTLASS_END_ACC_Y", 0xff},
	{"RH_SHORT_D_CUTLASS_END_SPD_XZ", 0x100},
	{"RH_SHORT_AIR_GLADIUS_HIT_SPD_Y", 0x101},
	{"RH_SHORT_AIR_GLADIUS_HIT_ACC_Y", 0x102},
	{"RH_SHORT_AIR_GLADIUS_THROW_SPD_Y", 0x103},
	{"RH_SHORT_AIR_GLADIUS_THROW_ACC_Y", 0x104},
	{"RH_SHORT_AIR_GLADIUS_D_HIT_SPD_Y", 0x105},
	{"RH_SHORT_AIR_GLADIUS_D_HIT_ACC_Y", 0x106},
	{"RH_SHORT_AIR_GLADIUS_D_THROW_SPD_Y", 0x107},
	{"RH_SHORT_AIR_GLADIUS_D_THROW_ACC_Y", 0x108},
	{"RH_SHORT_AIR_BASILISK_SPD_Y", 0x109},
	{"RH_SHORT_AIR_BASILISK_ACC_Y", 0x10a},
	{"RH_SHORT_AIR_BASILISK_SPD_Y2", 0x10b},
	{"RH_SHORT_AIR_BASILISK_ACC_Y2", 0x10c},
	{"RH_SHORT_AIR_BASILISK_SPD_Y3", 0x10d},
	{"RH_SHORT_AIR_BASILISK_ACC_Y3", 0x10e},
	{"RH_SG_BAEL_HIT_SLOW_RATE", 0x10f},
	{"RH_SG_BAEL_HIT_SLOW_TIME", 0x110},
	{"RH_SG_BAEL_POS_MOVE_TIME", 0x111},
	{"RH_SG_BAEL_F_HIT_SLOW_RATE", 0x112},
	{"RH_SG_BAEL_F_HIT_SLOW_TIME", 0x113},
	{"RH_SG_ECHIDNA_CHANCE_TIME", 0x114},
	{"RH_SG_ECHIDNA_CHANCE_TIME_D", 0x115},
	{"RH_SG_ECHIDNA_HIT_SLOW_RATE", 0x116},
	{"RH_SG_ECHIDNA_HIT_SLOW_TIME", 0x117},
	{"RH_SG_ECHIDNA_OVIDUCT_HIT_SPD_Y", 0x118},
	{"RH_SG_ECHIDNA_OVIDUCT_HIT_ACC_Y", 0x119},
	{"RH_SG_ECHIDNA_OVIDUCT_LP_SPD_Y", 0x11a},
	{"RH_SG_ECHIDNA_OVIDUCT_LP_ACC_Y", 0x11b},
	{"RH_SG_ECHIDNA_OVIDUCT_UPPER_SPD_Y", 0x11c},
	{"RH_SG_ECHIDNA_OVIDUCT_UPPER_ACC_Y", 0x11d},
	{"RH_SG_ECHIDNA_OVIDUCT_END_SPD_Y", 0x11e},
	{"RH_SG_ECHIDNA_OVIDUCT_END_ACC_Y", 0x11f},
	{"RH_SG_ECHIDNA_OVIDUCT_D_MOT_SPD", 0x120},
	{"RH_SHORT_AIR_CREDO_FAIL_HIT_SPD_Y", 0x121},
	{"RH_SHORT_AIR_CREDO_FAIL_HIT_ACC_Y", 0x122},
	{"RH_SHORT_AIR_CREDO_HIT_SPD_Y", 0x123},
	{"RH_SHORT_AIR_CREDO_HIT_ACC_Y", 0x124},
	{"RH_SHORT_AIR_CREDO_THROW_SPD_Y", 0x125},
	{"RH_SHORT_AIR_CREDO_THROW_ACC_Y", 0x126},
	{"RH_SHORT_AIR_CREDO_DOWN_SPD_Y", 0x127},
	{"RH_SHORT_AIR_CREDO_DOWN_ACC_Y", 0x128},
	{"RH_SHORT_AIR_ALTO_SPEAR_HIT_SPD_Y", 0x129},
	{"RH_SHORT_AIR_ALTO_SPEAR_HIT_ACC_Y", 0x12a},
	{"RH_SHORT_AIR_ALTO_SPEAR_THROW_SPD_Y", 0x12b},
	{"RH_SHORT_AIR_ALTO_SPEAR_THROW_ACC_Y", 0x12c},
	{"RH_SHORT_AGNUS_DOWN_SPD_Y", 0x12d},
	{"RH_SHORT_AGNUS_DOWN_ACC_Y", 0x12e},
	{"RH_SHORT_AGNUS_DOWN_SPD_Z", 0x12f},
	{"RH_SHORT_AGNUS_HIT_SPD_Y", 0x130},
	{"RH_SHORT_AGNUS_HIT_ACC_Y", 0x131},
	{"RH_SHORT_AIR_GOD_HIT_SPD_Y", 0x132},
	{"RH_SHORT_AIR_GOD_HIT_ACC_Y", 0x133},
	{"RH_SHORT_AIR_GOD_HIT_SPD_Y2", 0x134},
	{"RH_SHORT_AIR_GOD_HIT_ACC_Y2", 0x135},
	{"RH_SHORT_AIR_D_GOD_HIT_SPD_Y", 0x136},
	{"RH_SHORT_AIR_D_GOD_HIT_ACC_Y", 0x137},
	{"RH_SHORT_AIR_D_GOD_HIT_SPD_Y2", 0x138},
	{"RH_SHORT_AIR_D_GOD_HIT_ACC_Y2", 0x139},
	{"RH_SGA_POPE_HIT_SPD_Y", 0x13a},
	{"RH_SGA_POPE_HIT_ACC_Y", 0x13b},
	{"RH_SGA_POPE_DOWN_SPD_Y", 0x13c},
	{"RH_SGA_POPE_DOWN_ACC_Y", 0x13d},
	{"RH_SG_POPE_HIT_SLOW_RATE", 0x13e},
	{"RH_SG_POPE_HIT_SLOW_TIME", 0x13f},
	{"RH_SGA_DPOPE_HIT_SPD_Y", 0x140},
	{"RH_SGA_DPOPE_HIT_ACC_Y", 0x141},
	{"RH_SGA_DPOPE_DOWN_SPD_Y", 0x142},
	{"RH_SGA_DPOPE_DOWN_ACC_Y", 0x143},
	{"RH_SG_DPOPE_HIT_SLOW_RATE", 0x144},
	{"RH_SG_DPOPE_HIT_SLOW_TIME", 0x145},
	{"RH_SG_DPOPE_F_HIT_SLOW_RATE", 0x146},
	{"RH_SG_DPOPE_F_HIT_SLOW_TIME", 0x147},
	{"RH_SGA_DPOPE_HIT_SLOW_RATE", 0x148},
	{"RH_SGA_DPOPE_HIT_SLOW_TIME", 0x149},
	{"RH_SGA_DPOPE_FIRST_SLOW_RATE", 0x14a},
	{"RH_SGA_DPOPE_FIRST_SLOW_TIME", 0x14b},
	{"RH_SG_DPOPE_FIRST_SLOW_RATE", 0x14c},
	{"RH_SG_DPOPE_FIRST_SLOW_TIME", 0x14d},
	{"RH_ENEMY_BLOCK_BGN_SPD_Y", 0x14e},
	{"RH_ENEMY_BLOCK_BGN_ACC_Y", 0x14f},
	{"RH_ENEMY_BLOCK_GUARD_STOP_LV0", 0x150},
	{"RH_ENEMY_BLOCK_GUARD_STOP_LV1", 0x151},
	{"RH_ENEMY_BLOCK_GUARD_STOP_LV2", 0x152},
	{"RH_ENEMY_BLOCK_GUARD_ANGLE", 0x153},
	{"EG_DEVIL_ROTATION_SPD_MAX", 0x154},
	{"EG_GUARD_ANGLE_EM000", 0x155},
	{"EG_GUARD_ANGLE_EM001", 0x156},
	{"EG_GUARD_ANGLE_EM003", 0x157},
	{"EG_GUARD_ANGLE_EM008", 0x158},
	{"EG_GUARD_ANGLE_EM009", 0x159},
	{"EG_GUARD_ANGLE_EM010", 0x15a},
	{"EG_GUARD_ANGLE_EM011", 0x15b},
	{"EG_GUARD_ANGLE_EM013", 0x15c},
	{"EG_GUARD_ANGLE_EM015", 0x15d},
	{"EG_GUARD_ANGLE_EM016", 0x15e},
	{"EG_GUARD_ANGLE_EM017", 0x15f},
};

static const ParamInfo danteParams[] = {
	{"ATCK_STINGER_LV1_TIME", 0x0},
	{"ATCK_STINGER_LV1_RANGE", 0x1},
	{"ATCK_STINGER_LV2_TIME", 0x2},
	{"ATCK_STINGER_LV2_RANGE", 0x3},
	{"ATCK_STINGER_MOT_MOVE_LEN", 0x4},
	{"ATCK_STINGER_JUMP_XZ_SPD", 0x5},
	{"ATCK_STINGER_JUMP_Y_SPD", 0x6},
	{"ATCK_STINGER_JUMP_Y_ADD", 0x7},
	{"ATCK_AIR_GUN_XZ_SPD", 0x8},
	{"ATCK_AIR_GUN_Y_SPD", 0x9},
	{"ATCK_AIR_GUN_Y_ADD", 0xa},
	{"ATCK_AIR_GUN_2ND_Y_SPD", 0xb},
	{"ATCK_AIR_GUN_2ND_Y_ADD", 0xc},
	{"ATCK_AIR_GUN_2ND_ADD_SPD", 0xd},
	{"ATCK_AIR_GUN_XZ_SPD_MAX", 0xe},
	{"AUTO_LOCK_HOLD_TIME_GUN", 0xf},
	{"ATCK_HITIME_JUMP_SPD", 0x10},
	{"ATCK_HITIME_JUMP_ACC", 0x11},
	{"SRV_AIR_COMBO0_SPD_Y", 0x12},
	{"SRV_AIR_COMBO0_ADD_Y", 0x13},
	{"SRV_AIR_COMBO1_SPD_Y", 0x14},
	{"SRV_AIR_COMBO1_ADD_Y", 0x15},
	{"SRV_AIR_COMBO2_SPD_Y", 0x16},
	{"SRV_AIR_COMBO2_ADD_Y", 0x17},
	{"SRV_AIR_COMBO3_SPD_Y", 0x18},
	{"SRV_AIR_COMBO3_ADD_Y", 0x19},
	{"ATCK_SEI_RSTRM_Y_SPD", 0x1a},
	{"ATCK_SEI_RSTRM_Y_ADD", 0x1b},
	{"ATCK_SEI_RSTRM_END_HEIGHT", 0x1c},
	{"WP003_SHELL00_SPEED", 0x1d},
	{"WP003_SHELL00_LEN", 0x1e},
	{"WP023_SHELL_LV2_LEN", 0x1f},
	{"WP023_SHELL_LV3_LEN", 0x20},
	{"WP003_SHELL02_SPEED", 0x21},
	{"WP003_SHELL02_LEN", 0x22},
	{"WP003_SHELL03_SPEED", 0x23},
	{"WP003_SHELL03_LEN", 0x24},
	{"WP003_SHELL04_SPEED", 0x25},
	{"WP003_SHELL04_LEN", 0x26},
	{"ATCK_AIR_CAGUN_XZ_SPD", 0x27},
	{"ATCK_AIR_CAGUN_Y_SPD", 0x28},
	{"ATCK_AIR_CAGUN_Y_ADD", 0x29},
	{"ATCK_AIR_CAGUN_2ND_Y_SPD", 0x2a},
	{"ATCK_AIR_CAGUN_2ND_Y_ADD", 0x2b},
	{"ATCK_AIR_CAGUN_2ND_ADD_SPD", 0x2c},
	{"ATCK_AIR_CAGUN_XZ_SPD_MAX", 0x2d},
	{"AUTO_LOCK_HOLD_TIME_CAGUN", 0x2e},
	{"AIR_FLIPPED_Y_SPD", 0x2f},
	{"AIR_FLIPPED_Y_ADD", 0x30},
	{"AIR_FLIPPED_XZ_SPD", 0x31},
	{"AIR_FLIPPED_XZ_ADD", 0x32},
	{"SCA_NUNCHAKUS_SPD_Y", 0x33},
	{"SCA_NUNCHAKUS_ADD_Y", 0x34},
	{"WP010_SHELL05_SPEED", 0x35},
	{"WP010_SHELL05_LEN", 0x36},
	{"WP010_SHELL05_RANGE0", 0x37},
	{"WP010_SHELL05_RANGE1", 0x38},
	{"WP010_SHELL06_SPEED", 0x39},
	{"WP010_SHELL06_LEN", 0x3a},
	{"WP010_SHELL06_RANGE0", 0x3b},
	{"WP010_SHELL06_RANGE1", 0x3c},
	{"WP010_SHELL07_SPEED", 0x3d},
	{"WP010_SHELL07_LEN", 0x3e},
	{"WP010_SHELL07_RANGE0", 0x3f},
	{"WP010_SHELL07_RANGE1", 0x40},
	{"WP010_SHELL08_SPEED", 0x41},
	{"WP010_SHELL08_LEN", 0x42},
	{"WP010_SHELL08_RANGE0", 0x43},
	{"WP010_SHELL08_RANGE1", 0x44},
	{"WP010_SHELL09_SPEED", 0x45},
	{"WP010_SHELL09_LEN", 0x46},
	{"WP010_SHELL09_RANGE0", 0x47},
	{"WP010_SHELL09_RANGE1", 0x48},
	{"WP010_SHELL05_LV2_LEN", 0x49},
	{"WP010_SHELL05_LV3_LEN", 0x4a},
	{"TS_AIR_DASH_ADD_Y", 0x4b},
	{"TS_AIR_DASH_ADD_XZ", 0x4c},
	{"TS_ATRICK_HIDE_TIME", 0x4d},
	{"TS_ATRICK_LENGTH_MAX", 0x4e},
	{"TS_ATRICK_LENGTH_SUB", 0x4f},
	{"TS_ATRICK_HEIGHT_SUB", 0x50},
	{"TS_ATRICK_SPD_XZ", 0x51},
	{"TS_ATRICK_SPD_Y", 0x52},
	{"TS_ATRICK_ADD_Y", 0x53},
	{"KABUTO_HEIGHT_LIMIT", 0x54},
	{"RAIN_STORM_HEIGHT_LIMIT", 0x55},
	{"AIR_COMBO_HEIGHT_LIMIT", 0x56},
	{"DESCENT_KICK_HEIGHT_LIMIT", 0x57},
	{"AIR_ATCK_COMMON_ADD_Y_SPD", 0x58},
	{"WP003_SHELL_LV2_LEN", 0x59},
	{"WP003_SHELL_LV3_LEN", 0x5a},
	{"WP003_SHELL02_ATTACK_RAD", 0x5b},
	{"WP003_SHELL10_SPEED", 0x5c},
	{"WP003_SHELL10_LEN", 0x5d},
	{"HITIME_MOVE_EM_DIST", 0x5e},
	{"ATCK_RV_KABUTO_LEN0", 0x5f},
	{"ATCK_RV_KABUTO_LEN1", 0x60},
	{"ATCK_RV_ROUNDTRIP_TIME", 0x61},
	{"SRV_AIR_RETURN_SPD_Y", 0x62},
	{"SRV_AIR_RETURN_ADD_Y", 0x63},
	{"ATCK_AIR_CNT_EI_SHOT", 0x64},
	{"ATCK_AIR_CNT_SCA_SHOT", 0x65},
	{"ATCK_AIR_CNT_SEI_RSTRM", 0x66},
	{"ATCK_AIR_CNT_SCA_NUNCHAKUS", 0x67},
	{"ATCK_AIR_CNT_SRV_ACOMBO0", 0x68},
	{"ATCK_AIR_CNT_SRV_ACOMBO1", 0x69},
	{"ATCK_AIR_CNT_SRV_ACOMBO2", 0x6a},
	{"ATCK_AIR_CNT_SRV_ACOMBO3", 0x6b},
	{"ATCK_AIR_CNT_RV_RETURN", 0x6c},
	{"ATCK_HYAKU_MIN_TIME", 0x6d},
	{"ATCK_HYAKU_MAX_TIME1", 0x6e},
	{"ATCK_HYAKU_MAX_TIME2", 0x6f},
	{"ATCK_HYAKU_MAX_TIME3", 0x70},
	{"SRV_ROLLING_MIN_TIME", 0x71},
	{"SRV_ROLLING_MAX_TIME1", 0x72},
	{"SRV_ROLLING_MAX_TIME2", 0x73},
	{"SRV_ROLLING_MAX_TIME3", 0x74},
	{"ATCK_RV_KABUTO_LV0_SPD_Y", 0x75},
	{"ATCK_RV_KABUTO_LV0_ADD_Y", 0x76},
	{"ATCK_RV_KABUTO_LV1_SPD_Y", 0x77},
	{"ATCK_RV_KABUTO_LV1_ADD_Y", 0x78},
	{"ATCK_RV_KABUTO_LV2_SPD_Y", 0x79},
	{"ATCK_RV_KABUTO_LV2_ADD_Y", 0x7a},
	{"ATCK_SRV_DRIVE_LEN", 0x7b},
	{"ATCK_SRV_DRIVE00_SPD", 0x7c},
	{"ATCK_SRV_DRIVE01_SPD", 0x7d},
	{"ATCK_SRV_DRIVE02_SPD", 0x7e},
	{"ATCK_SRV_DRIVE_HEIGHT_ROT", 0x7f},
	{"WP023_SHELL11_SPEED", 0x80},
	{"WP023_SHELL11_LEN", 0x81},
	{"ATCK_AIR_PD_Y_SPD", 0x82},
	{"ATCK_AIR_PD_Y_ADD", 0x83},
	{"ATCK_ID_SCA_STINGER_TIME", 0x84},
	{"ATCK_ID_SCA_STINGER_RANGE", 0x85},
	{"ATCK_GM_STRAIGHT_TIME", 0x86},
	{"ATCK_GM_STRAIGHT_RANGE", 0x87},
	{"SGM_RISING_DRAGON_SPD_Y", 0x88},
	{"SGM_RISING_DRAGON_ADD_Y", 0x89},
	{"SGM_RISING_DRAGON_TIME", 0x8a},
	{"SGM_DIVINE_DRAGON_SPD_Y", 0x8b},
	{"SGM_DIVINE_DRAGON_ADD_Y", 0x8c},
	{"SGM_DIVINE_DRAGON_TIME", 0x8d},
	{"SGM_DIVINE_DRAGON_LOOP_NUM", 0x8e},
	{"ATCK_CS_HOLD_TIME_LV0", 0x8f},
	{"ATCK_CS_HOLD_TIME_LV1", 0x90},
	{"ATCK_CS_HOLD_TIME_LV2", 0x91},
	{"RG_AIR_BLOCK_SUCCESS_SPD_Y", 0x92},
	{"RG_AIR_BLOCK_SUCCESS_ADD_Y", 0x93},
	{"RG_AIR_BLOCK_JUST_SPD_Y", 0x94},
	{"RG_AIR_BLOCK_JUST_ADD_Y", 0x95},
	{"RG_AIR_BLOCK_ATCK_SPD_Y", 0x96},
	{"RG_AIR_BLOCK_ATCK_ADD_Y", 0x97},
	{"RG_GUAGE_MAX_LV0", 0x98},
	{"RG_GUAGE_MAX_LV1", 0x99},
	{"RG_GUAGE_MAX_LV2", 0x9a},
	{"RG_GUAGE_MAX_LV3", 0x9b},
	{"ATCK_CS_CA_HOLD_TIME_LV0", 0x9c},
	{"ATCK_CS_CA_HOLD_TIME_LV1", 0x9d},
	{"ATCK_CS_CA_HOLD_TIME_LV2", 0x9e},
	{"WP003_SHELL00_LEN_BOSS", 0x9f},
	{"WP003_SHELL02_LEN_BOSS", 0xa0},
	{"WP003_SHELL03_LEN_BOSS", 0xa1},
	{"WP003_SHELL04_LEN_BOSS", 0xa2},
	{"WP010_SHELL05_LEN_BOSS", 0xa3},
	{"WP010_SHELL06_LEN_BOSS", 0xa4},
	{"WP010_SHELL07_LEN_BOSS", 0xa5},
	{"WP010_SHELL08_LEN_BOSS", 0xa6},
	{"WP010_SHELL09_LEN_BOSS", 0xa7},
	{"WP003_SHELL10_LEN_BOSS", 0xa8},
	{"ATCK_SRV_DRIVE_LEN_BOSS", 0xa9},
	{"WP023_SHELL11_LEN_BOSS", 0xaa},
	{"RG_BLOCK_ADD_RG_GAUGE", 0xab},
	{"RG_BLOCK_ADD_RG_GAUGE_JUST", 0xac},
	{"RG_BLOCK_ADD_RG_GAUGE_CRUSH", 0xad},
	{"RG_BLOCK_COMBO_UP_BONUS", 0xae},
	{"RG_BLOCK_COMBO_UP_BONUS_JUST", 0xaf},
	{"RG_RELEASE_COMBO_UP_BONUS_JUST", 0xb0},
	{"RG_RELEASE_COMBO_UP_BONUS_GUARD", 0xb1},
	{"RG_BLOCK_COMBO_UP_BONUS_TIMER", 0xb2},
	{"RG_BLOCK_COMBO_UP_BONUS_JUST_TIMER", 0xb3},
	{"RG_RELEASE_COMBO_UP_BONUS_JUST_TIMER", 0xb4},
	{"RG_RELEASE_COMBO_UP_BONUS_GUARD_TIMER", 0xb5},
	{"RG_ADD_RG_GAUGE_PARAM_A", 0xb6},
	{"RG_ADD_RG_GAUGE_PARAM_B", 0xb7},
	{"RG_ADD_RG_GAUGE_BONUS_JUST", 0xb8},
	{"RG_ADD_RG_GAUGE_BONUS_GUARD", 0xb9},
	{"RG_BLOCK_KNOCK_BACK_SPD_XZ", 0xba},
	{"RG_BLOCK_KNOCK_BACK_ADD_XZ", 0xbb},
	{"RG_BLOCK_GUARD_KNOCK_BACK_SPD_XZ", 0xbc},
	{"RG_BLOCK_GUARD_KNOCK_BACK_ADD_XZ", 0xbd},
	{"RG_BLOCK_AIR_KNOCK_BACK_SPD_XZ", 0xbe},
	{"RG_BLOCK_AIR_KNOCK_BACK_ADD_XZ", 0xbf},
	{"RG_BLOCK_GUARD_AIR_KNOCK_BACK_SPD_XZ", 0xc0},
	{"RG_BLOCK_GUARD_AIR_KNOCK_BACK_ADD_XZ", 0xc1},
	{"ATCK_LF_AIR_COMBO_SPD_Y", 0xc2},
	{"ATCK_LF_AIR_COMBO_ADD_Y", 0xc3},
	{"ATCK_LF_AIR_COMBO_SPD2_Y", 0xc4},
	{"ATCK_LF_AIR_COMBO_ADD2_Y", 0xc5},
	{"ATCK_AIR_CNT_LF_COMBO", 0xc6},
	{"ATCK_GM_SAVE_TIME_LV1", 0xc7},
	{"ATCK_GM_SAVE_TIME_LV2", 0xc8},
	{"SGM_REAL_IMPACT_SPD_Y", 0xc9},
	{"SGM_REAL_IMPACT_ADD_Y", 0xca},
	{"SGM_INFERNO_SPD_Y", 0xcb},
	{"SGM_INFERNO_ADD_Y", 0xcc},
	{"ATCK_AIR_CNT_SGM_INFERNO", 0xcd},
	{"ATCK_PD_SHOT_SAVE_TIME_LV1", 0xce},
	{"ATCK_PD_SHOT_SAVE_TIME_LV2", 0xcf},
	{"PD_GUAGE_MAX_LV0", 0xd0},
	{"PD_GUAGE_MAX_LV1", 0xd1},
	{"PD_GUAGE_MAX_LV2", 0xd2},
	{"PD_GUAGE_MAX_LV3", 0xd3},
	{"ATCK_PD_METEOR_GUAGE_FIX_TIME", 0xd4},
	{"WP023_SHELL14_SPEED", 0xd5},
	{"WP023_SHELL14_LEN", 0xd6},
	{"WP023_SHELL14_LEN_BOSS", 0xd7},
	{"WP023_SHELL15_SPEED", 0xd8},
	{"WP023_SHELL15_LEN", 0xd9},
	{"WP023_SHELL15_LEN_BOSS", 0xda},
	{"WP023_SHELL16_SPEED", 0xdb},
	{"WP023_SHELL16_LEN", 0xdc},
	{"WP023_SHELL16_LEN_BOSS", 0xdd},
	{"WP023_SHELL17_SPEED", 0xde},
	{"WP023_SHELL17_LEN", 0xdf},
	{"WP023_SHELL17_LEN_BOSS", 0xe0},
	{"ATCK_PD_METEOR_SHOT_TIME", 0xe1},
	{"GM_KICK_13R_DVL_SPD_Y", 0xe2},
	{"GM_KICK_13R_DVL_ADD_Y", 0xe3},
	{"ATCK_AIR_CNT_MGUN_REPEAT", 0xe4},
	{"DANTE_DEVIL_TRG_DOWN_VAL", 0xe5},
	{"ATCK_PD_MGUN_SHOT_TIME", 0xe6},
	{"PD_GUAGE_DOWN_METEOR", 0xe7},
	{"PD_GUAGE_DOWN_METEOR_SHOT", 0xe8},
	{"PD_GUAGE_DOWN_METEOR_SHOT2", 0xe9},
	{"PD_GUAGE_DOWN_BOOMERANG", 0xea},
	{"ATCK_PD_BOOMERANG_GUAGE_FIX_TIME", 0xeb},
	{"ATCK_PD_BOOMERANG_HIT_TIME", 0xec},
	{"ATCK_PD_BOOMERANG_HIT_ENABLE_TIME", 0xed},
	{"ATCK_SRV_DRIVE_UP_TIME_LV1", 0xee},
	{"ATCK_SRV_DRIVE_UP_TIME_LV2", 0xef},
	{"PD_GUAGE_UP_LAUNCHER", 0xf0},
	{"PD_GUAGE_UP_LAUNCHER2", 0xf1},
	{"PD_GUAGE_UP_LAUNCHER3", 0xf2},
	{"PD_GUAGE_UP_MACHINE_GUN", 0xf3},
	{"WP003_SHELL03_TARGET_INNER", 0xf4},
	{"WP003_SHELL10_TARGET_INNER", 0xf5},
	{"WP023_SHELL15_TARGET_INNER", 0xf6},
	{"WP010_SHELL05_TARGET_INNER", 0xf7},
	{"ATCK_PD_METEOR_S_SPD_Y", 0xf8},
	{"ATCK_PD_METEOR_S_ADD_Y", 0xf9},
	{"ATCK_PD_METEOR_E_SPD_Y", 0xfa},
	{"ATCK_PD_METEOR_E_ADD_Y", 0xfb},
	{"RG_INVINCIBLE_FIX_TIME", 0xfc},
	{"RG_INVINCIBLE_GUAGE_DOWN", 0xfd},
	{"RG_INVINCIBLE_END_TIME", 0xfe},
	{"RG_INVINCIBLE_HITSTOP_TIME00", 0xff},
	{"RG_INVINCIBLE_JOINT_VIB_UP00", 0x100},
	{"RG_INVINCIBLE_JOINT_VIB_DOWN00", 0x101},
	{"RG_INVINCIBLE_JOINT_VIB_FRAME00", 0x102},
	{"RG_INVINCIBLE_HITSTOP_TIME01", 0x103},
	{"RG_INVINCIBLE_JOINT_VIB_UP01", 0x104},
	{"RG_INVINCIBLE_JOINT_VIB_DOWN01", 0x105},
	{"RG_INVINCIBLE_JOINT_VIB_FRAME01", 0x106},
	{"TS_AIR_ESC_KNEEL_FRAME_S", 0x107},
	{"TS_AIR_ESC_KNEEL_FRAME_Y", 0x108},
	{"TS_AIR_ESC_KNEEL_FRAME_E", 0x109},
	{"TS_AIR_ESC_COMBO_FRAME_S", 0x10a},
	{"TS_AIR_ESC_COMBO_FRAME_Y", 0x10b},
	{"TS_AIR_ESC_COMBO_FRAME_E", 0x10c},
	{"TS_AIR_ESC_HITIME_FRAME_S", 0x10d},
	{"TS_AIR_ESC_HITIME_FRAME_Y", 0x10e},
	{"TS_AIR_ESC_HITIME_FRAME_E", 0x10f},
	{"TS_AIR_ESC_COMBO_UP_BONUS", 0x110},
	{"TS_AIR_ESC_COMBO_UP_BONUS_TIME", 0x111},
	{"ATCK_AIR_CNT_PD_METEOR", 0x112},
	{"ATCK_YT_SPACE_EFC_X", 0x113},
	{"ATCK_YT_SPACE_EFC_Y", 0x114},
	{"ATCK_YT_SPACE_EFC_Z", 0x115},
	{"ATCK_LF_SHL_REARRANGEMENT_TIME", 0x116},
	{"ATCK_LF_SHL_WAIT_BURST_TIME", 0x117},
	{"ATCK_LF_SHL_DETONATE_TIME", 0x118},
	{"ATCK_LF_SHL_DETONATE_INTERVAL_TIME", 0x119},
	{"ATCK_LF_SHL_INTENSIVE_TIME", 0x11a},
	{"ATCK_LF_SHL_INTENSIVE_INTERVA_TIME", 0x11b},
	{"ATCK_LF_SHL_INTENSIVE_SPD", 0x11c},
	{"ATCK_LF_SHL_EM_CNST_BURST_TIME", 0x11d},
	{"ATCK_YT_AIR_COMBO0_SPD_Y", 0x11e},
	{"ATCK_YT_AIR_COMBO0_ADD_Y", 0x11f},
	{"ATCK_AIR_CNT_YT_COMBO0", 0x120},
	{"ATCK_SCA_NUNCHAKUS_ID_PROBABILITY", 0x121},
	{"ATCK_YT_AIR_COMBO1_SPD_Y", 0x122},
	{"ATCK_YT_AIR_COMBO1_ADD_Y", 0x123},
	{"ATCK_AIR_CNT_YT_COMBO1", 0x124},
	{"ATCK_LF_AIR_DETONATE_ALL_SPD_Y", 0x125},
	{"ATCK_LF_AIR_DETONATE_ALL_ADD_Y", 0x126},
	{"ATCK_AIR_CNT_LF_DETONATE_ALL", 0x127},
	{"ATCK_YT_COMBO2_COMBO_UP_BONUS", 0x128},
	{"ATCK_YT_COMBO2_COMBO_UP_BONUS_TIMER", 0x129},
	{"ATCK_YT_COMBO2_DT_UP_BONUS", 0x12a},
	{"ATCK_YT_SPACE2_EFC_X", 0x12b},
	{"ATCK_YT_SPACE2_EFC_Y", 0x12c},
	{"ATCK_YT_SPACE2_EFC_Z", 0x12d},
	{"ATCK_LF_CROSS_THROW_TIME00", 0x12e},
	{"ATCK_LF_CROSS_THROW_TIME01", 0x12f},
	{"ATCK_LF_AIR_COMBO_THROW_TIME00", 0x130},
	{"ATCK_LF_AIR_COMBO_THROW_TIME01", 0x131},
	{"ATCK_LF_AIR_COMBO_THROW_TIME02", 0x132},
	{"DT_HP_RECOVER", 0x133},
	{"ATCK_LF_COMBO0_TIME00", 0x134},
	{"ATCK_LF_COMBO2_TIME00", 0x135},
	{"ATCK_LF_COMBO2_TIME01", 0x136},
	{"ATCK_LF_DIO_KNIFE_TIME00", 0x137},
	{"ATCK_LF_DIO_KNIFE_TIME01", 0x138},
	{"ATCK_LF_DIO_KNIFE_TIME02", 0x139},
	{"ATCK_LF_DT_DIO_KNIFE_TIME00", 0x13a},
	{"ATCK_LF_DT_DIO_KNIFE_TIME01", 0x13b},
	{"ATCK_LF_DT_DIO_KNIFE_TIME02", 0x13c},
	{"ATCK_LF_SHL_BOM_TIME", 0x13d},
	{"ATCK_LF_SHL_BOM_INTERVAL_TIME", 0x13e},
	{"DANTE_DEVIL_TRG_DOWN_VAL2", 0x13f},
	{"ATCK_SGM_INFERNO_HEIGHT_LIMIT", 0x140},
	{"ATCK_DS_AIR_COMBO0_HEIGHT_LIMIT", 0x141},
	{"ATCK_DS_AIR_COMBO1_HEIGHT_LIMIT", 0x142},
	{"ATCK_LF_BOM_BARRIER_SPD_Y", 0x143},
	{"ATCK_LF_BOM_BARRIER_ADD_Y", 0x144},
	{"ATCK_AIR_CNT_LF_BOM_BARRIER", 0x145},
	{"DT_GAUGE_KEEP_FRM", 0x146},
	{"RG_BLOCK_DEVIL_UP_BONUS", 0x147},
	{"RG_BLOCK_DEVIL_UP_BONUS_JUST", 0x148},
	{"RG_RELEASE_DEVIL_UP_BONUS_JUST", 0x149},
	{"RG_RELEASE_DEVIL_UP_BONUS_GUARD", 0x14a},
	{"TS_AIR_ESC_DEVIL_UP_BONUS", 0x14b},
	{"GM_JUST_SAVE_COMBO_UP_BONUS_TIME", 0x14c},
	{"GM_JUST_SAVE_COMBO_UP_BONUS", 0x14d},
	{"GM_JUST_SAVE_DEVIL_UP_BONUS", 0x14e},
	{"PD_BOX_LV0_COMBO_UP_BONUS_TIME", 0x14f},
	{"PD_BOX_LV0_COMBO_UP_BONUS", 0x150},
	{"PD_BOX_LV0_DEVIL_UP_BONUS", 0x151},
	{"PD_BOX_LV1_COMBO_UP_BONUS_TIME", 0x152},
	{"PD_BOX_LV1_COMBO_UP_BONUS", 0x153},
	{"PD_BOX_LV1_DEVIL_UP_BONUS", 0x154},
	{"PD_BOX_LV2_COMBO_UP_BONUS_TIME", 0x155},
	{"PD_BOX_LV2_COMBO_UP_BONUS", 0x156},
	{"PD_BOX_LV2_DEVIL_UP_BONUS", 0x157},
	{"PD_BOX_LV3_COMBO_UP_BONUS_TIME", 0x158},
	{"PD_BOX_LV3_COMBO_UP_BONUS", 0x159},
	{"PD_BOX_LV3_DEVIL_UP_BONUS", 0x15a},
	{"DT_GUAGE_DOWN_STINGER", 0x15b},
	{"DT_GUAGE_DOWN_KICK13", 0x15c},
	{"DT_GUAGE_DOWN_DIO_KNIFE", 0x15d},
};


template<typename ParamArray, size_t N>
void DisplayParameters(void* baseAddress, const ParamArray (&params)[N], const char* windowTitle) {
    float* floatArray = (float*)baseAddress;
    static std::unordered_map<std::string, std::vector<float>> backupMap;
    static std::unordered_map<std::string, bool> hasBackupMap;
    
    std::string key(windowTitle);
    auto& backupValues = backupMap[key];
    auto& hasBackup = hasBackupMap[key];
    
    ImGui::Begin(windowTitle);
    ImGui::SetWindowFontScale(0.8f);
    
    if (ImGui::Button("Save Current Values")) {
        backupValues.clear();
        backupValues.reserve(N);
        for (const auto& param : params) {
            backupValues.push_back(floatArray[param.offset]);
        }
        hasBackup = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Apply Saved Values")) {
        if (hasBackup && backupValues.size() > 0) {
            size_t i = 0;
            for (const auto& param : params) {
                if (i < backupValues.size()) {
                    floatArray[param.offset] = backupValues[i];
                    i++;
                }
            }
        }
    }

    ImGui::BeginChild("ValuesChild");
    ImGui::PushItemWidth(100);
    for (size_t i = 0; i < N; ++i) {
        float& value = floatArray[params[i].offset];
        ImGui::InputFloat(params[i].name, &value);
    }
    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::End();
}

void PlayerTracker::on_frame(fmilliseconds& dt) {
	if (show_player_params) {
		uPlayer* player = devil4_sdk::get_local_player();
		if (!player) { return; }
		if (player->controllerID == 0) {
			DisplayParameters(player->charactersettingsThree, danteParams, "Dante Parameters");
		}
		else {
			DisplayParameters(player->charactersettingsThree, neroParams, "Nero Parameters");
		}
	}
}

void PlayerTracker::SavePlayerXYZ() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        savedPlayerPosition = player->m_pos;
        savedPlayerRotation = player->rotation2;
    }
}
void PlayerTracker::LoadPlayerXYZ() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        player->m_pos = savedPlayerPosition;
        player->rotation2 = savedPlayerRotation;
    }
}

void PlayerTracker::SavePlayerMove() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        savedPlayerSword           = player->currentSword;
        savedPlayerGun             = player->currentGun;
        savedPlayerStyle           = player->currentStyle;
        savedPlayerMoveBank        = player->moveBank;
        savedPlayerMoveID          = player->moveID2;
        savedPlayerWeight          = player->weight;
        savedPlayerInertia         = player->inertia;
        savedPlayerCancels1[0]     = player->cancels1[0];
        savedPlayerCancels1[1]     = player->cancels1[1];
        savedPlayerCancels1[2]     = player->cancels1[2];
        savedPlayerCancels1[3]     = player->cancels1[3];
        savedPlayerCancels2[0]     = player->cancels2[0];
        savedPlayerCancels2[1]     = player->cancels2[1];
        savedPlayerCancels2[2]     = player->cancels2[2];
        savedPlayerCancels2[3]     = player->cancels2[3];
        savedPlayerLockonAnimation = player->isLockonAnimation;
        savedPlayerCanWeaponChange = player->canWeaponChange;
        SavePlayerXYZ();
    }
}

void PlayerTracker::LoadPlayerMove() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player && savedPlayerMoveID) { // verify at least 1 save has happened
        player->nextSword         = savedPlayerSword;
        player->nextGun           = savedPlayerGun;
        player->currentStyle      = savedPlayerStyle;
        player->moveBank          = savedPlayerMoveBank;
        player->moveID2           = savedPlayerMoveID;
        player->weight            = savedPlayerWeight;
        player->inertia           = savedPlayerInertia;
        player->movePart          = 0;
        player->cancels1[0]       = savedPlayerCancels1[0];
        player->cancels1[1]       = savedPlayerCancels1[1];
        player->cancels1[2]       = savedPlayerCancels1[2];
        player->cancels1[3]       = savedPlayerCancels1[3];
        player->cancels2[0]       = savedPlayerCancels2[0]; // fixes grounded saves not being loaded in the air correctly
        player->cancels2[1]       = savedPlayerCancels2[1];
        player->cancels2[2]       = savedPlayerCancels2[2];
        player->cancels2[3]       = savedPlayerCancels2[3];
        player->isLockonAnimation = savedPlayerLockonAnimation; // fixes not being able to load aerial animations while in the grounded lockon animation
        player->canWeaponChange   = savedPlayerCanWeaponChange;

        // grounded does not need to be set
        // player->characterSettingsOne->groundedActual = 0;
        // player->grounded = 0;
        // player->grounded2 = 0;
        LoadPlayerXYZ();
    }
}

std::optional<std::string> PlayerTracker::on_initialize() {
    MutatorRegistry::define("PocketKing").on_init([] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(0.5f, 0.5f, 0.5f);
        }
    }).set_timer(30.0f, [] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    });

    MutatorRegistry::define("BigGuy").on_init([] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.35f, 1.35f, 1.35f);
        }
    }).set_timer(30.0f, [] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    });

    console->system().RegisterCommand("playerdt", "Set DT value", 
        [](float value) {
            if (uPlayer* pl_ptr = devil4_sdk::get_local_player()) {
                pl_ptr->DT = value;
            }
        }, 
        csys::Arg<float>("0.0 - 10000.0"));

    console->system().RegisterCommand("playerhp", "Set HP value", 
        [](float value) {
            if (uPlayer* pl_ptr = devil4_sdk::get_local_player()) {
                pl_ptr->damageStruct.HP = value;
            }
        }, 
        csys::Arg<float>("0.0 - 20000.0"));

    return Mod::on_initialize();
}

// void PlayerTracker::on_update_input(utility::Input & input) {}

static uintptr_t moveIDCallAddr{ 0x7AAC80 };
static void PlayMoveID(int moveID) {
    auto player = devil4_sdk::get_local_player();
    _asm {
        mov eax, [player]
        push -1
        mov ecx, [moveID]
        call dword ptr [moveIDCallAddr]
    }
}

static uintptr_t animIDCallAddr{ 0x7AA030 };
static void PlayAnimID(int animID) {
    auto player = devil4_sdk::get_local_player();
    _asm {
        mov ecx, [player]
        push dword ptr [animID]
        call dword ptr [animIDCallAddr]
    }
}

void PlayerTracker::on_gui_frame(int display) {
	ImGui::Checkbox(_("Show Player Params"), &show_player_params);
    if (ImGui::CollapsingHeader(_("Shadow Settings"))) {
        sUnit* sUnit = devil4_sdk::get_sUnit();
        if (sUnit) {
            if (sUnit->shadow) {
                ImGui::Indent(lineIndent);
                ImGui::InputFloat3(_("Rotation"), &sUnit->shadow->rotation.x);
                ImGui::InputFloat4(_("Colour"), &sUnit->shadow->colour.x);
                ImGui::InputFloat(_("diffuse"), &sUnit->shadow->diffuse);
                ImGui::InputFloat(_("bloom"), &sUnit->shadow->bloom);
                ImGui::Unindent();
            }
        }
    }
    if (ImGui::CollapsingHeader(_("[OLD] Display Player Stats"))) {
        uPlayer* player = devil4_sdk::get_local_player();
        SMediator* s_med_ptr = devil4_sdk::get_sMediator();
        if (player) {
            ImGui::Indent(lineIndent);
            if (ImGui::Button(_("Save Current Move"))) {
                SavePlayerMove();
            }
            ImGui::SameLine();
            if (ImGui::Button(_("Play Saved Move"))) {
                LoadPlayerMove();
            }
            ImGui::InputFloat3(_("Saved Player Position"), &savedPlayerPosition[0]);

            static int inputAnimID = 0;
            static bool loopAnimID = false;
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::InputInt("##InputAnimIDInputInt ##1", &inputAnimID);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Play Animation ID")) {
                uPlayer* player = devil4_sdk::get_local_player();
                if (!player) { return; }
                player->movePart = 3;
                devil4_sdk::indexed_anim_call(inputAnimID, player, 0, 1.0f, 0.0f, 3.0f);
                if (!loopAnimID)
                    player->playMoveOnce = 4;
            }
            ImGui::SameLine();
            help_marker(_("uhh this only plays things that don't require a certain player state, "
                "e.g. You must have just taken damage to play a damage animation"));
            ImGui::SameLine();
            ImGui::Checkbox(_("Loop"), &loopAnimID);
            ImGui::NewLine();
            ImGui::SliderFloat(_("HP ##1"), &player->damageStruct.HP, 0.0f, 20000.0f, "%.0f");
            ImGui::SliderFloat(_("Max HP ##1"), &player->damageStruct.HPMax, 0.0f, 20000.0f, "%.0f");
            ImGui::SliderFloat(_("DT ##1"), &player->DT, 0.0f, 10000.0f, "%.0f");
            ImGui::SliderFloat(_("Max DT ##1"), &player->maxDT, 0.0f, 10000.0f, "%.0f");
            ImGui::InputFloat(_("Previous Hit Dealt"), &player->damageStruct.HPTaken);
            ImGui::InputInt(_("Stun 1##2"), &player->damageStruct.stun[0]);
            ImGui::InputInt(_("Stun 2##2"), &player->damageStruct.stun[1]);
            ImGui::InputInt(_("Stun 3##2"), &player->damageStruct.stun[2]);
            ImGui::InputInt(_("Stun 4##2"), &player->damageStruct.stun[3]);
            ImGui::InputInt(_("Stun 5##2"), &player->damageStruct.stun[4]);
            ImGui::InputInt(_("Displacement 1##2"), &player->damageStruct.displacement[0]);
            ImGui::InputInt(_("Displacement 2##2"), &player->damageStruct.displacement[1]);
            ImGui::InputInt(_("Displacement 3##2"), &player->damageStruct.displacement[2]);
            ImGui::InputInt(_("Displacement 4##2"), &player->damageStruct.displacement[3]);
            ImGui::InputInt(_("Displacement 5##2"), &player->damageStruct.displacement[4]);
            ImGui::InputInt(_("Unknown 1##2"), &player->damageStruct.unknown[0]);
            ImGui::InputInt(_("Unknown 2##2"), &player->damageStruct.unknown[1]);
            ImGui::InputInt(_("Unknown 3##2"), &player->damageStruct.unknown[2]);
            ImGui::InputInt(_("Unknown 4##2"), &player->damageStruct.unknown[3]);
            ImGui::InputInt(_("Unknown 5##2"), &player->damageStruct.unknown[4]);
            ImGui::InputInt(_("Unknown 6##2"), &player->damageStruct.unknown[5]);
            ImGui::InputInt(_("Unknown 7##2"), &player->damageStruct.unknown[6]);
            ImGui::InputInt(_("Unknown 8##2"), &player->damageStruct.unknown[7]);
            ImGui::InputFloat(_("BP Timer ##1"), &s_med_ptr->bpTimer);
            ImGui::InputFloat3(_("XYZ Position ##1"), (float*)&player->m_pos);
            ImGui::SliderFloat(_("Rotation ##1"), &player->rotation2, -3.14f, 3.14f, "%.2f");
            ImGui::InputFloat3(_("XYZ Scale ##1"), (float*)&player->m_scale);
            ImGui::InputFloat3(_("XYZ Velocity ##1"), (float*)&player->m_d_velocity);
            ImGui::InputFloat(_("Movement Speed ##1"), &player->m_d_vel_magnitude);
            ImGui::InputFloat(_("Inertia ##1"), &player->inertia);
            ImGui::SameLine();
            help_marker(_("Uhm, ehm, akshually, internia isn't a thing, it's a property of a thing, the shit you're showing is velocity, the "
                "measure of inertia is mass, resistance to acceleration (slowing down is also acceleration just in the opposite "
                "direction), if the thing that got yeeted doesn't like burn off it's layers in flight due to air friction like a "
                "fucking meteor or some shit then it's inertia isn't changing. Get it right NERD"));
            ImGui::InputScalar(_("Weight ##1"), ImGuiDataType_U8, &player->weight);
            ImGui::InputScalar(_("Lock On ##1"), ImGuiDataType_U8, &player->lockedOn);
            if (player->controllerID == 0) { // dante
                ImGui::SliderFloat(_("Disaster Gauge ##1"), &player->disasterGauge, 0.0f, 10000.0f, "%.0f");
                ImGui::SliderFloat(_("Revenge Gauge ##1"), &player->revengeGauge, 0.0f, 30000.0f, "%.0f");
            }
            else { // nero
                ImGui::InputFloat(_("Exceed Timer ##1"), &player->exceedTimer, 0.0f, 16.0f, "%.1f");
                ImGui::SameLine();
                help_marker(_("If you press exceed while this timer is between 0 and 1, you'll get MAX-Act."));
            }
            ImGui::SliderFloat(_("Animation Frame ##1"), &player->animFrame, 0.0f, player->animFrameMax, "%.1f");
            ImGui::InputScalar(_("Animation ID ##1"), ImGuiDataType_U16, &player->animID);
            ImGui::InputScalar(_("Move ID ##1"), ImGuiDataType_U32, &player->moveIDBest);
            ImGui::InputScalar(_("Move ID2 ##1"), ImGuiDataType_U32, &player->moveID2);
            ImGui::InputScalar(_("Move Bank ##1"), ImGuiDataType_U32, &player->moveBank);
            ImGui::InputScalar(_("Move Part ##1"), ImGuiDataType_U32, &player->movePart);
            ImGui::InputScalar(_("Saved Move Bank ##1"), ImGuiDataType_U8, &savedPlayerMoveBank);
            ImGui::InputScalar(_("Saved Move ID ##1"), ImGuiDataType_U8, &savedPlayerMoveID);
            ImGui::Unindent(lineIndent);
        }
	}
}

// void PlayerTracker::on_config_save(utility::Config& cfg) {}

// void PlayerTracker::on_config_load(const utility::Config& cfg) {}

#endif
