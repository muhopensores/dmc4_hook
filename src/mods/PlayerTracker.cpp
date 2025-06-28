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
	{__("AIR_ATCK_COMMON_ADD_Y_SPD"), 0x0},
	{__("AIR_FLIPPED_Y_SPD"), 0x1},
	{__("AIR_FLIPPED_Y_ADD"), 0x2},
	{__("AIR_FLIPPED_XZ_SPD"), 0x3},
	{__("AIR_FLIPPED_XZ_ADD"), 0x4},
	{__("DT_GAUGE_DOWN"), 0x5},
	{__("DT_GAUGE_DOWN_POWER_UP"), 0x6},
	{__("DT_HP_RECOVER"), 0x7},
	{__("DT_GAUGE_KEEP_FRM"), 0x8},
	{__("DT_BUSTER_RATE"), 0x9},
	{__("DROPKICK_SPD_Z"), 0xa},
	{__("DROPKICK_ACC_Y"), 0xb},
	{__("AIR_DEVIL_TRG_SPD_Y"), 0xc},
	{__("AIR_DEVIL_TRG_ACC_Y"), 0xd},
	{__("RH_LONG_GRAB_STYLISH_POINT"), 0xe},
	{__("RH_SHORT_GRAB_STYLISH_POINT"), 0xf},
	{__("EG_GUARD_STYLISH_PTS"), 0x10},
	{__("EG_GUARD_STYLISH_TIMER"), 0x11},
	{__("EG_GUARD_DT_ADD"), 0x12},
	{__("RQ_AIR_COMBO0_SPD_XZ"), 0x13},
	{__("RQ_AIR_COMBO0_SPD_Y"), 0x14},
	{__("RQ_AIR_COMBO0_ADD_Y"), 0x15},
	{__("RQ_AIR_COMBO1_SPD_XZ"), 0x16},
	{__("RQ_AIR_COMBO1_SPD_Y"), 0x17},
	{__("RQ_AIR_COMBO1_ADD_Y"), 0x18},
	{__("RQ_AIR_COMBO2_SPD_XZ"), 0x19},
	{__("RQ_AIR_COMBO2_SPD_Y"), 0x1a},
	{__("RQ_AIR_COMBO2_ADD_Y"), 0x1b},
	{__("AIR_COMBO_HEIGHT_LIMIT"), 0x1c},
	{__("RQ_AIR_DELAY_SPD_Y"), 0x1d},
	{__("RQ_AIR_DELAY_ADD_Y"), 0x1e},
	{__("RQ_AIR_DELAY_HEIGHT_LIMIT"), 0x1f},
	{__("RQ_COMBO_B_SPD1"), 0x20},
	{__("RQ_COMBO_B_SPD2"), 0x21},
	{__("RQ_COMBO_B_SPD3"), 0x22},
	{__("RQ_COMBO_B_SPD4"), 0x23},
	{__("RQ_COMBO_B_SPD5"), 0x24},
	{__("RQ_COMBO_B_SPD6"), 0x25},
	{__("HITIME_JUMP_SPD"), 0x26},
	{__("HITIME_JUMP_ACC"), 0x27},
	{__("HITIME_MOVE_EM_DIST"), 0x28},
	{__("ATCK_STINGER_LV1_TIME"), 0x29},
	{__("ATCK_STINGER_LV1_RANGE"), 0x2a},
	{__("ATCK_STINGER_LV2_TIME"), 0x2b},
	{__("ATCK_STINGER_LV2_RANGE"), 0x2c},
	{__("ATCK_STINGER_MOT_MOVE_LEN"), 0x2d},
	{__("ATCK_STINGER_JUMP_XZ_SPD"), 0x2e},
	{__("ATCK_STINGER_JUMP_Y_SPD"), 0x2f},
	{__("ATCK_STINGER_JUMP_Y_ADD"), 0x30},
	{__("KABUTO_HEIGHT_LIMIT"), 0x31},
	{__("OD_HITIME_JUMP1_SPD"), 0x32},
	{__("OD_HITIME_JUMP1_ACC"), 0x33},
	{__("OD_HITIME_JUMP2_SPD"), 0x34},
	{__("OD_HITIME_JUMP2_ACC"), 0x35},
	{__("OD_HITIME_JUMP3_SPD"), 0x36},
	{__("OD_HITIME_JUMP3_ACC"), 0x37},
	{__("RQ_OD_HITIME_HITSLOW_RATE"), 0x38},
	{__("RQ_OD_HITIME_HITSLOW_TIME"), 0x39},
	{__("RQ_OD_STINGER_CHARGE_TIME"), 0x3a},
	{__("RQ_OD_STINGER_LV1_S_TIME"), 0x3b},
	{__("RQ_OD_STINGER_LV1_S_LEN"), 0x3c},
	{__("RQ_OD_STINGER_LV1_L_TIME"), 0x3d},
	{__("RQ_OD_STINGER_LV1_L_LEN"), 0x3e},
	{__("RQ_OD_STINGER_LV2_S_TIME"), 0x3f},
	{__("RQ_OD_STINGER_LV2_S_LEN"), 0x40},
	{__("RQ_OD_STINGER_LV2_L_TIME"), 0x41},
	{__("RQ_OD_STINGER_LV2_L_LEN"), 0x42},
	{__("RQ_OD_STINGER_LV3_S_TIME"), 0x43},
	{__("RQ_OD_STINGER_LV3_S_LEN"), 0x44},
	{__("RQ_OD_STINGER_LV3_L_TIME"), 0x45},
	{__("RQ_OD_STINGER_LV3_L_LEN"), 0x46},
	{__("RQ_OD_STINGER_SPD_DOWN_RATE"), 0x47},
	{__("DEMO_STINGER_TIME"), 0x48},
	{__("DEMO_STINGER_RANGE"), 0x49},
	{__("RQ_AIR_STINGER_TIME"), 0x4a},
	{__("RQ_AIR_STINGER_RANGE"), 0x4b},
	{__("RQ_AIR_STINGER_HEIGHT_LIMIT"), 0x4c},
	{__("RQ_AIR_STINGER_XZ_SPD_DECELERATE"), 0x4d},
	{__("RQ_FLIP_S_BACK_Z_SPD"), 0x4e},
	{__("RQ_FLIP_S_BACK_Z_ACC"), 0x4f},
	{__("RQ_FLIP_L_BACK_Z_SPD"), 0x50},
	{__("RQ_FLIP_L_BACK_Z_ACC"), 0x51},
	{__("OD_GUAGE_SPD"), 0x52},
	{__("OD_GUAGE_ADD"), 0x53},
	{__("OD_GUAGE_SPD_MAX"), 0x54},
	{__("OD_GUAGE_SUB"), 0x55},
	{__("OD_GUAGE_MAX"), 0x56},
	{__("OD_FOLLOW_THROUGH_REFUSE_FRM"), 0x57},
	{__("OD_AIR_HEIGHT_LIMIT"), 0x58},
	{__("OD_AIR_SPD_Y"), 0x59},
	{__("OD_AIR_ADD_Y"), 0x5a},
	{__("OD_AIR_SPD_Y_ADD"), 0x5b},
	{__("ATCK_AIR_GUN_XZ_SPD"), 0x5c},
	{__("ATCK_AIR_GUN_Y_SPD"), 0x5d},
	{__("ATCK_AIR_GUN_Y_ADD"), 0x5e},
	{__("ATCK_AIR_GUN_2ND_Y_SPD"), 0x5f},
	{__("ATCK_AIR_GUN_2ND_Y_ADD"), 0x60},
	{__("ATCK_AIR_GUN_XZ_SPD_MAX"), 0x61},
	{__("BR_LOCK_HOLD_TIME"), 0x62},
	{__("BR_AIR_XZ_SPD_RATE"), 0x63},
	{__("WP000_SHELL_LV2_LEN"), 0x64},
	{__("WP000_SHELL_LV3_LEN"), 0x65},
	{__("BR_AIR_CSHOT1_SPD_XZ"), 0x66},
	{__("BR_AIR_CSHOT1_RATE_XZ"), 0x67},
	{__("BR_AIR_CSHOT1_SPD_Y"), 0x68},
	{__("BR_AIR_CSHOT1_ADD_Y"), 0x69},
	{__("BR_AIR_CSHOT2_SPD_XZ"), 0x6a},
	{__("BR_AIR_CSHOT2_RATE_XZ"), 0x6b},
	{__("BR_AIR_CSHOT2_SPD_Y"), 0x6c},
	{__("BR_AIR_CSHOT2_ADD_Y"), 0x6d},
	{__("WP000_SHELL_SPEED"), 0x6e},
	{__("WP000_SHELL_LEN"), 0x6f},
	{__("WP000_SHELL_LEN_BOSS"), 0x70},
	{__("DEVIL_DRIVE_SHELL00_SPEED"), 0x71},
	{__("DEVIL_DRIVE_SHELL00_LEN"), 0x72},
	{__("DEVIL_DRIVE_SHELL00_LEN_BOSS"), 0x73},
	{__("DEVIL_DRIVE_SHELL_SPEED"), 0x74},
	{__("DEVIL_DRIVE_SHELL_LEN"), 0x75},
	{__("DEVIL_DRIVE_SHELL_LEN_BOSS"), 0x76},
	{__("DEVIL_DRIVE_SHELL_OFF_TIME"), 0x77},
	{__("DEVIL_DRIVE_SHELL_ADD_SPEED"), 0x78},
	{__("DEVIL_DRIVE_SHELL_MAX_SPEED"), 0x79},
	{__("DEVIL_DRIVE_SHELL_HIT_NUM00"), 0x7a},
	{__("DEVIL_DRIVE_SHELL_HIT_NUM01"), 0x7b},
	{__("DEVIL_DRIVE_SHELL_SHOT_TIME"), 0x7c},
	{__("ATCK_IS_HOLD_TIME"), 0x7d},
	{__("ATCK_IS_CSHOT_WAITTIME"), 0x7e},
	{__("ATCK_IS_SHOT_SPD"), 0x7f},
	{__("ATCK_IS_SHOT_ADD"), 0x80},
	{__("ATCK_IS_SHOT_ADD_MAX"), 0x81},
	{__("ATCK_IS_CSHOT_TIME"), 0x82},
	{__("ATCK_IS_ENABLE_TIME"), 0x83},
	{__("ATCK_IS_ENABLE_TIME_BOSS"), 0x84},
	{__("RH_LONG_GRAB_DIST_LV1"), 0x85},
	{__("RH_LONG_GRAB_DIST_LV2"), 0x86},
	{__("RH_LONG_GRAB_DIST_LV3"), 0x87},
	{__("AIR_LONG_GRAB_HEIGHT_LIMIT"), 0x88},
	{__("RH_LONG_AIR_GRAB_Y_SPD"), 0x89},
	{__("RH_LONG_AIR_GRAB_Y_ADD"), 0x8a},
	{__("RH_LONG_EM_DRAW_POS_X"), 0x8b},
	{__("RH_LONG_EM_DRAW_POS_Y"), 0x8c},
	{__("RH_LONG_EM_DRAW_POS_Z"), 0x8d},
	{__("RH_LONG_EM_SHORT_DRAW_POS_X"), 0x8e},
	{__("RH_LONG_EM_SHORT_DRAW_POS_Y"), 0x8f},
	{__("RH_LONG_EM_SHORT_DRAW_POS_Z"), 0x90},
	{__("RH_LONG_APPROACH_XZ_SPD_MAX"), 0x91},
	{__("RH_LONG_APPROACH_XZ_SPD_MIN"), 0x92},
	{__("RH_LONG_APPROACH_XZ_SPD_DECELERATE"), 0x93},
	{__("DEMO_RH_LONG_APPROACH_XZ_SPD_MAX"), 0x94},
	{__("DEMO_RH_LONG_APPROACH_XZ_SPD_MIN"), 0x95},
	{__("DEMO_RH_LONG_APPROACH_XZ_LEN_MAX"), 0x96},
	{__("DEMO_RH_LONG_APPROACH_XZ_LEN_MIN"), 0x97},
	{__("DEMO_RH_LONG_APPROACH_XZ_SPD_DECELERATE"), 0x98},
	{__("LG_AIR_FLIPPED_XZ_SPD"), 0x99},
	{__("LG_AIR_FLIPPED_XZ_RATE"), 0x9a},
	{__("LG_AIR_FLIPPED_Y_SPD"), 0x9b},
	{__("LG_AIR_FLIPPED_Y_ADD"), 0x9c},
	{__("AIR_SHORT_GRAB_HEIGHT_LIMIT"), 0x9d},
	{__("RH_SHORT_AIR_GRAB_Y_SPD"), 0x9e},
	{__("RH_SHORT_AIR_GRAB_Y_ADD"), 0x9f},
	{__("RH_SG_ANG_ADJUST_DIST"), 0xa0},
	{__("RH_SG_ANG_ADJUST_HEIGHT"), 0xa1},
	{__("RH_SG_ANG_ADJUST_BOTTOM"), 0xa2},
	{__("RH_SG_ANG_ADJUST_ANG"), 0xa3},
	{__("RH_SG_ANG_ADJUST_AT_DIST"), 0xa4},
	{__("RH_SG_ANG_ADJUST_AT_HEIGHT"), 0xa5},
	{__("RH_SG_ANG_ADJUST_AT_BOTTOM"), 0xa6},
	{__("RH_SHORT_AIR_GRAB_HIT_Y_SPD"), 0xa7},
	{__("RH_SHORT_AIR_GRAB_HIT_Y_ADD"), 0xa8},
	{__("RH_SHORT_AIR_GRAB_THROW_Y_SPD"), 0xa9},
	{__("RH_SHORT_AIR_GRAB_THROW_Y_ADD"), 0xaa},
	{__("RH_SHORT_AIR_SCAREBALL_HIT_SPD_Y"), 0xab},
	{__("RH_SHORT_AIR_SCAREBALL_HIT_ACC_Y"), 0xac},
	{__("RH_SHORT_AIR_SCAREBALL_ROLL_SPD_Y"), 0xad},
	{__("RH_SHORT_AIR_SCAREBALL_ROLL_ACC_Y"), 0xae},
	{__("RH_SHORT_AIR_SCAREBALL_END_SPD_Y"), 0xaf},
	{__("RH_SHORT_AIR_SCAREBALL_END_ACC_Y"), 0xb0},
	{__("RH_SHORT_AIR_BIANCO_HIT_SPD_Y"), 0xb1},
	{__("RH_SHORT_AIR_BIANCO_HIT_ACC_Y"), 0xb2},
	{__("RH_SHORT_AIR_BIANCO_STICK_SPD_Y"), 0xb3},
	{__("RH_SHORT_AIR_BIANCO_STICK_ACC_Y"), 0xb4},
	{__("RH_SHORT_AIR_BIANCO_BURST_SPD_Y"), 0xb5},
	{__("RH_SHORT_AIR_BIANCO_BURST_ACC_Y"), 0xb6},
	{__("RH_SHORT_BIANCO_MOT_SPD"), 0xb7},
	{__("RH_SHORT_BIANCO_MOT_SPD2"), 0xb8},
	{__("RH_SGD_BIANCO_MV_FRM_1"), 0xb9},
	{__("RH_SGD_BIANCO_MV_DIST_1"), 0xba},
	{__("RH_SGD_BIANCO_MV_FRM_2"), 0xbb},
	{__("RH_SGD_BIANCO_MV_DIST_2"), 0xbc},
	{__("RH_SGD_BIANCO_MV_FRM_3"), 0xbd},
	{__("RH_SGD_BIANCO_MV_DIST_3"), 0xbe},
	{__("RH_SGAD_BIANCO_DOWN_SPD_Y"), 0xbf},
	{__("RH_SHORT_AIR_BIANCO_PIERCE_SPD_Y"), 0xc0},
	{__("RH_SHORT_AIR_BIANCO_PIERCE_ACC_Y"), 0xc1},
	{__("RH_SGD_BIANCO_MAX_ANG"), 0xc2},
	{__("RH_SGD_BIANCO_BURST_SPD_Y"), 0xc3},
	{__("RH_SGD_BIANCO_BURST_ACC_Y"), 0xc4},
	{__("RH_SG_BIANCO_BURST_SPD_XZ"), 0xc5},
	{__("RH_SGD_BIANCO_BURST_SPD_XZ"), 0xc6},
	{__("RH_SHORT_AIR_ALTO_HIT_SPD_Y"), 0xc7},
	{__("RH_SHORT_AIR_ALTO_HIT_ACC_Y"), 0xc8},
	{__("RH_SHORT_AIR_ALTO_LIFT_SPD_Y"), 0xc9},
	{__("RH_SHORT_AIR_ALTO_LIFT_ACC_Y"), 0xca},
	{__("RH_SHORT_AIR_ALTO_DOWN_SPD_Y"), 0xcb},
	{__("RH_SHORT_AIR_ALTO_DOWN_ACC_Y"), 0xcc},
	{__("RH_SHORT_AIR_ALTO_D_DOWN_SPD_Y"), 0xcd},
	{__("RH_SHORT_AIR_ALTO_D_DOWN_ACC_Y"), 0xce},
	{__("RH_SGA_ALTO_HIT_Z_SPD"), 0xcf},
	{__("RH_SGA_D_ALTO_DOWN_Z_SPD"), 0xd0},
	{__("RH_SG_ALTO_ROTATE_FRM"), 0xd1},
	{__("RH_SHORT_AIR_FROST_SPD_Y"), 0xd2},
	{__("RH_SHORT_AIR_FROST_ACC_Y"), 0xd3},
	{__("RH_SHORT_AIR_FROST_SPD_Y_LP"), 0xd4},
	{__("RH_SHORT_AIR_FROST_ACC_Y_LP"), 0xd5},
	{__("RH_SHORT_AIR_FROST_SPD_Y_LP2"), 0xd6},
	{__("RH_SHORT_AIR_FROST_ACC_Y_LP2"), 0xd7},
	{__("RH_SHORT_AIR_FROST_SPD_Y_END"), 0xd8},
	{__("RH_SHORT_AIR_FROST_ACC_Y_END"), 0xd9},
	{__("RH_SHORT_AIR_FROST_MOT_SPD"), 0xda},
	{__("RH_SHORT_AIR_FROST_MOT_SPD2"), 0xdb},
	{__("RH_SHORT_AIR_FROST_D_MOT_SPD"), 0xdc},
	{__("RH_SHORT_AIR_FROST_D_MOT_SPD2"), 0xdd},
	{__("RH_SHORT_AIR_ASSAULT_HIT_SPD_Y"), 0xde},
	{__("RH_SHORT_AIR_ASSAULT_HIT_ACC_Y"), 0xdf},
	{__("RH_SHORT_AIR_ASSAULT_DOWN_SPD_Y"), 0xe0},
	{__("RH_SHORT_AIR_ASSAULT_DOWN_ACC_Y"), 0xe1},
	{__("RH_SHORT_AIR_ASSAULT_MOT_SPD"), 0xe2},
	{__("RH_SHORT_AIR_ASSAULT_MOT_SPD2"), 0xe3},
	{__("RH_SHORT_ASSAULT_COUNTER_DOWN_SPD_Y"), 0xe4},
	{__("RH_SHORT_ASSAULT_COUNTER_DOWN_ACC_Y"), 0xe5},
	{__("RH_SHORT_ASSAULT_COUNTER_DOWN_SPD_XZ"), 0xe6},
	{__("RH_SHORT_ASSAULT_MOT_SPD"), 0xe7},
	{__("RH_SHORT_AIR_BLITZ_HIT_SPD_Y"), 0xe8},
	{__("RH_SHORT_AIR_BLITZ_HIT_ACC_Y"), 0xe9},
	{__("RH_SHORT_AIR_BLITZ_LP_SPD_Y"), 0xea},
	{__("RH_SHORT_AIR_BLITZ_LP_ACC_Y"), 0xeb},
	{__("RH_SHORT_AIR_BLITZ_END1_SPD_Y"), 0xec},
	{__("RH_SHORT_AIR_BLITZ_END1_ACC_Y"), 0xed},
	{__("RH_SHORT_AIR_BLITZ_END2_SPD_Y"), 0xee},
	{__("RH_SHORT_AIR_BLITZ_END2_ACC_Y"), 0xef},
	{__("RH_SHORT_BLITZ_D_MOT_SPD"), 0xf0},
	{__("RH_SGA_CHIMAIRASEED_HIT_SPD_Y"), 0xf1},
	{__("RH_SGA_CHIMAIRASEED_HIT_ACC_Y"), 0xf2},
	{__("RH_SGA_CHIMAIRASEED_PULL_SPD_Y"), 0xf3},
	{__("RH_SGA_CHIMAIRASEED_PULL_ACC_Y"), 0xf4},
	{__("RH_SG_CHIMAIRASEED_MOT_SPD"), 0xf5},
	{__("RH_SHORT_CUTLASS_DOWN_SPD_Y"), 0xf6},
	{__("RH_SHORT_CUTLASS_DOWN_ACC_Y"), 0xf7},
	{__("RH_SHORT_CUTLASS_DOWN_SPD_XZ"), 0xf8},
	{__("RH_SHORT_AIR_CUTLASS_HIT_SPD_Y"), 0xf9},
	{__("RH_SHORT_AIR_CUTLASS_HIT_ACC_Y"), 0xfa},
	{__("RH_SHORT_CUTLASS_END_SPD_Y"), 0xfb},
	{__("RH_SHORT_CUTLASS_END_ACC_Y"), 0xfc},
	{__("RH_SHORT_CUTLASS_END_SPD_XZ"), 0xfd},
	{__("RH_SHORT_D_CUTLASS_END_SPD_Y"), 0xfe},
	{__("RH_SHORT_D_CUTLASS_END_ACC_Y"), 0xff},
	{__("RH_SHORT_D_CUTLASS_END_SPD_XZ"), 0x100},
	{__("RH_SHORT_AIR_GLADIUS_HIT_SPD_Y"), 0x101},
	{__("RH_SHORT_AIR_GLADIUS_HIT_ACC_Y"), 0x102},
	{__("RH_SHORT_AIR_GLADIUS_THROW_SPD_Y"), 0x103},
	{__("RH_SHORT_AIR_GLADIUS_THROW_ACC_Y"), 0x104},
	{__("RH_SHORT_AIR_GLADIUS_D_HIT_SPD_Y"), 0x105},
	{__("RH_SHORT_AIR_GLADIUS_D_HIT_ACC_Y"), 0x106},
	{__("RH_SHORT_AIR_GLADIUS_D_THROW_SPD_Y"), 0x107},
	{__("RH_SHORT_AIR_GLADIUS_D_THROW_ACC_Y"), 0x108},
	{__("RH_SHORT_AIR_BASILISK_SPD_Y"), 0x109},
	{__("RH_SHORT_AIR_BASILISK_ACC_Y"), 0x10a},
	{__("RH_SHORT_AIR_BASILISK_SPD_Y2"), 0x10b},
	{__("RH_SHORT_AIR_BASILISK_ACC_Y2"), 0x10c},
	{__("RH_SHORT_AIR_BASILISK_SPD_Y3"), 0x10d},
	{__("RH_SHORT_AIR_BASILISK_ACC_Y3"), 0x10e},
	{__("RH_SG_BAEL_HIT_SLOW_RATE"), 0x10f},
	{__("RH_SG_BAEL_HIT_SLOW_TIME"), 0x110},
	{__("RH_SG_BAEL_POS_MOVE_TIME"), 0x111},
	{__("RH_SG_BAEL_F_HIT_SLOW_RATE"), 0x112},
	{__("RH_SG_BAEL_F_HIT_SLOW_TIME"), 0x113},
	{__("RH_SG_ECHIDNA_CHANCE_TIME"), 0x114},
	{__("RH_SG_ECHIDNA_CHANCE_TIME_D"), 0x115},
	{__("RH_SG_ECHIDNA_HIT_SLOW_RATE"), 0x116},
	{__("RH_SG_ECHIDNA_HIT_SLOW_TIME"), 0x117},
	{__("RH_SG_ECHIDNA_OVIDUCT_HIT_SPD_Y"), 0x118},
	{__("RH_SG_ECHIDNA_OVIDUCT_HIT_ACC_Y"), 0x119},
	{__("RH_SG_ECHIDNA_OVIDUCT_LP_SPD_Y"), 0x11a},
	{__("RH_SG_ECHIDNA_OVIDUCT_LP_ACC_Y"), 0x11b},
	{__("RH_SG_ECHIDNA_OVIDUCT_UPPER_SPD_Y"), 0x11c},
	{__("RH_SG_ECHIDNA_OVIDUCT_UPPER_ACC_Y"), 0x11d},
	{__("RH_SG_ECHIDNA_OVIDUCT_END_SPD_Y"), 0x11e},
	{__("RH_SG_ECHIDNA_OVIDUCT_END_ACC_Y"), 0x11f},
	{__("RH_SG_ECHIDNA_OVIDUCT_D_MOT_SPD"), 0x120},
	{__("RH_SHORT_AIR_CREDO_FAIL_HIT_SPD_Y"), 0x121},
	{__("RH_SHORT_AIR_CREDO_FAIL_HIT_ACC_Y"), 0x122},
	{__("RH_SHORT_AIR_CREDO_HIT_SPD_Y"), 0x123},
	{__("RH_SHORT_AIR_CREDO_HIT_ACC_Y"), 0x124},
	{__("RH_SHORT_AIR_CREDO_THROW_SPD_Y"), 0x125},
	{__("RH_SHORT_AIR_CREDO_THROW_ACC_Y"), 0x126},
	{__("RH_SHORT_AIR_CREDO_DOWN_SPD_Y"), 0x127},
	{__("RH_SHORT_AIR_CREDO_DOWN_ACC_Y"), 0x128},
	{__("RH_SHORT_AIR_ALTO_SPEAR_HIT_SPD_Y"), 0x129},
	{__("RH_SHORT_AIR_ALTO_SPEAR_HIT_ACC_Y"), 0x12a},
	{__("RH_SHORT_AIR_ALTO_SPEAR_THROW_SPD_Y"), 0x12b},
	{__("RH_SHORT_AIR_ALTO_SPEAR_THROW_ACC_Y"), 0x12c},
	{__("RH_SHORT_AGNUS_DOWN_SPD_Y"), 0x12d},
	{__("RH_SHORT_AGNUS_DOWN_ACC_Y"), 0x12e},
	{__("RH_SHORT_AGNUS_DOWN_SPD_Z"), 0x12f},
	{__("RH_SHORT_AGNUS_HIT_SPD_Y"), 0x130},
	{__("RH_SHORT_AGNUS_HIT_ACC_Y"), 0x131},
	{__("RH_SHORT_AIR_GOD_HIT_SPD_Y"), 0x132},
	{__("RH_SHORT_AIR_GOD_HIT_ACC_Y"), 0x133},
	{__("RH_SHORT_AIR_GOD_HIT_SPD_Y2"), 0x134},
	{__("RH_SHORT_AIR_GOD_HIT_ACC_Y2"), 0x135},
	{__("RH_SHORT_AIR_D_GOD_HIT_SPD_Y"), 0x136},
	{__("RH_SHORT_AIR_D_GOD_HIT_ACC_Y"), 0x137},
	{__("RH_SHORT_AIR_D_GOD_HIT_SPD_Y2"), 0x138},
	{__("RH_SHORT_AIR_D_GOD_HIT_ACC_Y2"), 0x139},
	{__("RH_SGA_POPE_HIT_SPD_Y"), 0x13a},
	{__("RH_SGA_POPE_HIT_ACC_Y"), 0x13b},
	{__("RH_SGA_POPE_DOWN_SPD_Y"), 0x13c},
	{__("RH_SGA_POPE_DOWN_ACC_Y"), 0x13d},
	{__("RH_SG_POPE_HIT_SLOW_RATE"), 0x13e},
	{__("RH_SG_POPE_HIT_SLOW_TIME"), 0x13f},
	{__("RH_SGA_DPOPE_HIT_SPD_Y"), 0x140},
	{__("RH_SGA_DPOPE_HIT_ACC_Y"), 0x141},
	{__("RH_SGA_DPOPE_DOWN_SPD_Y"), 0x142},
	{__("RH_SGA_DPOPE_DOWN_ACC_Y"), 0x143},
	{__("RH_SG_DPOPE_HIT_SLOW_RATE"), 0x144},
	{__("RH_SG_DPOPE_HIT_SLOW_TIME"), 0x145},
	{__("RH_SG_DPOPE_F_HIT_SLOW_RATE"), 0x146},
	{__("RH_SG_DPOPE_F_HIT_SLOW_TIME"), 0x147},
	{__("RH_SGA_DPOPE_HIT_SLOW_RATE"), 0x148},
	{__("RH_SGA_DPOPE_HIT_SLOW_TIME"), 0x149},
	{__("RH_SGA_DPOPE_FIRST_SLOW_RATE"), 0x14a},
	{__("RH_SGA_DPOPE_FIRST_SLOW_TIME"), 0x14b},
	{__("RH_SG_DPOPE_FIRST_SLOW_RATE"), 0x14c},
	{__("RH_SG_DPOPE_FIRST_SLOW_TIME"), 0x14d},
	{__("RH_ENEMY_BLOCK_BGN_SPD_Y"), 0x14e},
	{__("RH_ENEMY_BLOCK_BGN_ACC_Y"), 0x14f},
	{__("RH_ENEMY_BLOCK_GUARD_STOP_LV0"), 0x150},
	{__("RH_ENEMY_BLOCK_GUARD_STOP_LV1"), 0x151},
	{__("RH_ENEMY_BLOCK_GUARD_STOP_LV2"), 0x152},
	{__("RH_ENEMY_BLOCK_GUARD_ANGLE"), 0x153},
	{__("EG_DEVIL_ROTATION_SPD_MAX"), 0x154},
	{__("EG_GUARD_ANGLE_EM000"), 0x155},
	{__("EG_GUARD_ANGLE_EM001"), 0x156},
	{__("EG_GUARD_ANGLE_EM003"), 0x157},
	{__("EG_GUARD_ANGLE_EM008"), 0x158},
	{__("EG_GUARD_ANGLE_EM009"), 0x159},
	{__("EG_GUARD_ANGLE_EM010"), 0x15a},
	{__("EG_GUARD_ANGLE_EM011"), 0x15b},
	{__("EG_GUARD_ANGLE_EM013"), 0x15c},
	{__("EG_GUARD_ANGLE_EM015"), 0x15d},
	{__("EG_GUARD_ANGLE_EM016"), 0x15e},
	{__("EG_GUARD_ANGLE_EM017"), 0x15f},
};

static const ParamInfo danteParams[] = {
	{__("ATCK_STINGER_LV1_TIME"), 0x0},
	{__("ATCK_STINGER_LV1_RANGE"), 0x1},
	{__("ATCK_STINGER_LV2_TIME"), 0x2},
	{__("ATCK_STINGER_LV2_RANGE"), 0x3},
	{__("ATCK_STINGER_MOT_MOVE_LEN"), 0x4},
	{__("ATCK_STINGER_JUMP_XZ_SPD"), 0x5},
	{__("ATCK_STINGER_JUMP_Y_SPD"), 0x6},
	{__("ATCK_STINGER_JUMP_Y_ADD"), 0x7},
	{__("ATCK_AIR_GUN_XZ_SPD"), 0x8},
	{__("ATCK_AIR_GUN_Y_SPD"), 0x9},
	{__("ATCK_AIR_GUN_Y_ADD"), 0xa},
	{__("ATCK_AIR_GUN_2ND_Y_SPD"), 0xb},
	{__("ATCK_AIR_GUN_2ND_Y_ADD"), 0xc},
	{__("ATCK_AIR_GUN_2ND_ADD_SPD"), 0xd},
	{__("ATCK_AIR_GUN_XZ_SPD_MAX"), 0xe},
	{__("AUTO_LOCK_HOLD_TIME_GUN"), 0xf},
	{__("ATCK_HITIME_JUMP_SPD"), 0x10},
	{__("ATCK_HITIME_JUMP_ACC"), 0x11},
	{__("SRV_AIR_COMBO0_SPD_Y"), 0x12},
	{__("SRV_AIR_COMBO0_ADD_Y"), 0x13},
	{__("SRV_AIR_COMBO1_SPD_Y"), 0x14},
	{__("SRV_AIR_COMBO1_ADD_Y"), 0x15},
	{__("SRV_AIR_COMBO2_SPD_Y"), 0x16},
	{__("SRV_AIR_COMBO2_ADD_Y"), 0x17},
	{__("SRV_AIR_COMBO3_SPD_Y"), 0x18},
	{__("SRV_AIR_COMBO3_ADD_Y"), 0x19},
	{__("ATCK_SEI_RSTRM_Y_SPD"), 0x1a},
	{__("ATCK_SEI_RSTRM_Y_ADD"), 0x1b},
	{__("ATCK_SEI_RSTRM_END_HEIGHT"), 0x1c},
	{__("WP003_SHELL00_SPEED"), 0x1d},
	{__("WP003_SHELL00_LEN"), 0x1e},
	{__("WP023_SHELL_LV2_LEN"), 0x1f},
	{__("WP023_SHELL_LV3_LEN"), 0x20},
	{__("WP003_SHELL02_SPEED"), 0x21},
	{__("WP003_SHELL02_LEN"), 0x22},
	{__("WP003_SHELL03_SPEED"), 0x23},
	{__("WP003_SHELL03_LEN"), 0x24},
	{__("WP003_SHELL04_SPEED"), 0x25},
	{__("WP003_SHELL04_LEN"), 0x26},
	{__("ATCK_AIR_CAGUN_XZ_SPD"), 0x27},
	{__("ATCK_AIR_CAGUN_Y_SPD"), 0x28},
	{__("ATCK_AIR_CAGUN_Y_ADD"), 0x29},
	{__("ATCK_AIR_CAGUN_2ND_Y_SPD"), 0x2a},
	{__("ATCK_AIR_CAGUN_2ND_Y_ADD"), 0x2b},
	{__("ATCK_AIR_CAGUN_2ND_ADD_SPD"), 0x2c},
	{__("ATCK_AIR_CAGUN_XZ_SPD_MAX"), 0x2d},
	{__("AUTO_LOCK_HOLD_TIME_CAGUN"), 0x2e},
	{__("AIR_FLIPPED_Y_SPD"), 0x2f},
	{__("AIR_FLIPPED_Y_ADD"), 0x30},
	{__("AIR_FLIPPED_XZ_SPD"), 0x31},
	{__("AIR_FLIPPED_XZ_ADD"), 0x32},
	{__("SCA_NUNCHAKUS_SPD_Y"), 0x33},
	{__("SCA_NUNCHAKUS_ADD_Y"), 0x34},
	{__("WP010_SHELL05_SPEED"), 0x35},
	{__("WP010_SHELL05_LEN"), 0x36},
	{__("WP010_SHELL05_RANGE0"), 0x37},
	{__("WP010_SHELL05_RANGE1"), 0x38},
	{__("WP010_SHELL06_SPEED"), 0x39},
	{__("WP010_SHELL06_LEN"), 0x3a},
	{__("WP010_SHELL06_RANGE0"), 0x3b},
	{__("WP010_SHELL06_RANGE1"), 0x3c},
	{__("WP010_SHELL07_SPEED"), 0x3d},
	{__("WP010_SHELL07_LEN"), 0x3e},
	{__("WP010_SHELL07_RANGE0"), 0x3f},
	{__("WP010_SHELL07_RANGE1"), 0x40},
	{__("WP010_SHELL08_SPEED"), 0x41},
	{__("WP010_SHELL08_LEN"), 0x42},
	{__("WP010_SHELL08_RANGE0"), 0x43},
	{__("WP010_SHELL08_RANGE1"), 0x44},
	{__("WP010_SHELL09_SPEED"), 0x45},
	{__("WP010_SHELL09_LEN"), 0x46},
	{__("WP010_SHELL09_RANGE0"), 0x47},
	{__("WP010_SHELL09_RANGE1"), 0x48},
	{__("WP010_SHELL05_LV2_LEN"), 0x49},
	{__("WP010_SHELL05_LV3_LEN"), 0x4a},
	{__("TS_AIR_DASH_ADD_Y"), 0x4b},
	{__("TS_AIR_DASH_ADD_XZ"), 0x4c},
	{__("TS_ATRICK_HIDE_TIME"), 0x4d},
	{__("TS_ATRICK_LENGTH_MAX"), 0x4e},
	{__("TS_ATRICK_LENGTH_SUB"), 0x4f},
	{__("TS_ATRICK_HEIGHT_SUB"), 0x50},
	{__("TS_ATRICK_SPD_XZ"), 0x51},
	{__("TS_ATRICK_SPD_Y"), 0x52},
	{__("TS_ATRICK_ADD_Y"), 0x53},
	{__("KABUTO_HEIGHT_LIMIT"), 0x54},
	{__("RAIN_STORM_HEIGHT_LIMIT"), 0x55},
	{__("AIR_COMBO_HEIGHT_LIMIT"), 0x56},
	{__("DESCENT_KICK_HEIGHT_LIMIT"), 0x57},
	{__("AIR_ATCK_COMMON_ADD_Y_SPD"), 0x58},
	{__("WP003_SHELL_LV2_LEN"), 0x59},
	{__("WP003_SHELL_LV3_LEN"), 0x5a},
	{__("WP003_SHELL02_ATTACK_RAD"), 0x5b},
	{__("WP003_SHELL10_SPEED"), 0x5c},
	{__("WP003_SHELL10_LEN"), 0x5d},
	{__("HITIME_MOVE_EM_DIST"), 0x5e},
	{__("ATCK_RV_KABUTO_LEN0"), 0x5f},
	{__("ATCK_RV_KABUTO_LEN1"), 0x60},
	{__("ATCK_RV_ROUNDTRIP_TIME"), 0x61},
	{__("SRV_AIR_RETURN_SPD_Y"), 0x62},
	{__("SRV_AIR_RETURN_ADD_Y"), 0x63},
	{__("ATCK_AIR_CNT_EI_SHOT"), 0x64},
	{__("ATCK_AIR_CNT_SCA_SHOT"), 0x65},
	{__("ATCK_AIR_CNT_SEI_RSTRM"), 0x66},
	{__("ATCK_AIR_CNT_SCA_NUNCHAKUS"), 0x67},
	{__("ATCK_AIR_CNT_SRV_ACOMBO0"), 0x68},
	{__("ATCK_AIR_CNT_SRV_ACOMBO1"), 0x69},
	{__("ATCK_AIR_CNT_SRV_ACOMBO2"), 0x6a},
	{__("ATCK_AIR_CNT_SRV_ACOMBO3"), 0x6b},
	{__("ATCK_AIR_CNT_RV_RETURN"), 0x6c},
	{__("ATCK_HYAKU_MIN_TIME"), 0x6d},
	{__("ATCK_HYAKU_MAX_TIME1"), 0x6e},
	{__("ATCK_HYAKU_MAX_TIME2"), 0x6f},
	{__("ATCK_HYAKU_MAX_TIME3"), 0x70},
	{__("SRV_ROLLING_MIN_TIME"), 0x71},
	{__("SRV_ROLLING_MAX_TIME1"), 0x72},
	{__("SRV_ROLLING_MAX_TIME2"), 0x73},
	{__("SRV_ROLLING_MAX_TIME3"), 0x74},
	{__("ATCK_RV_KABUTO_LV0_SPD_Y"), 0x75},
	{__("ATCK_RV_KABUTO_LV0_ADD_Y"), 0x76},
	{__("ATCK_RV_KABUTO_LV1_SPD_Y"), 0x77},
	{__("ATCK_RV_KABUTO_LV1_ADD_Y"), 0x78},
	{__("ATCK_RV_KABUTO_LV2_SPD_Y"), 0x79},
	{__("ATCK_RV_KABUTO_LV2_ADD_Y"), 0x7a},
	{__("ATCK_SRV_DRIVE_LEN"), 0x7b},
	{__("ATCK_SRV_DRIVE00_SPD"), 0x7c},
	{__("ATCK_SRV_DRIVE01_SPD"), 0x7d},
	{__("ATCK_SRV_DRIVE02_SPD"), 0x7e},
	{__("ATCK_SRV_DRIVE_HEIGHT_ROT"), 0x7f},
	{__("WP023_SHELL11_SPEED"), 0x80},
	{__("WP023_SHELL11_LEN"), 0x81},
	{__("ATCK_AIR_PD_Y_SPD"), 0x82},
	{__("ATCK_AIR_PD_Y_ADD"), 0x83},
	{__("ATCK_ID_SCA_STINGER_TIME"), 0x84},
	{__("ATCK_ID_SCA_STINGER_RANGE"), 0x85},
	{__("ATCK_GM_STRAIGHT_TIME"), 0x86},
	{__("ATCK_GM_STRAIGHT_RANGE"), 0x87},
	{__("SGM_RISING_DRAGON_SPD_Y"), 0x88},
	{__("SGM_RISING_DRAGON_ADD_Y"), 0x89},
	{__("SGM_RISING_DRAGON_TIME"), 0x8a},
	{__("SGM_DIVINE_DRAGON_SPD_Y"), 0x8b},
	{__("SGM_DIVINE_DRAGON_ADD_Y"), 0x8c},
	{__("SGM_DIVINE_DRAGON_TIME"), 0x8d},
	{__("SGM_DIVINE_DRAGON_LOOP_NUM"), 0x8e},
	{__("ATCK_CS_HOLD_TIME_LV0"), 0x8f},
	{__("ATCK_CS_HOLD_TIME_LV1"), 0x90},
	{__("ATCK_CS_HOLD_TIME_LV2"), 0x91},
	{__("RG_AIR_BLOCK_SUCCESS_SPD_Y"), 0x92},
	{__("RG_AIR_BLOCK_SUCCESS_ADD_Y"), 0x93},
	{__("RG_AIR_BLOCK_JUST_SPD_Y"), 0x94},
	{__("RG_AIR_BLOCK_JUST_ADD_Y"), 0x95},
	{__("RG_AIR_BLOCK_ATCK_SPD_Y"), 0x96},
	{__("RG_AIR_BLOCK_ATCK_ADD_Y"), 0x97},
	{__("RG_GUAGE_MAX_LV0"), 0x98},
	{__("RG_GUAGE_MAX_LV1"), 0x99},
	{__("RG_GUAGE_MAX_LV2"), 0x9a},
	{__("RG_GUAGE_MAX_LV3"), 0x9b},
	{__("ATCK_CS_CA_HOLD_TIME_LV0"), 0x9c},
	{__("ATCK_CS_CA_HOLD_TIME_LV1"), 0x9d},
	{__("ATCK_CS_CA_HOLD_TIME_LV2"), 0x9e},
	{__("WP003_SHELL00_LEN_BOSS"), 0x9f},
	{__("WP003_SHELL02_LEN_BOSS"), 0xa0},
	{__("WP003_SHELL03_LEN_BOSS"), 0xa1},
	{__("WP003_SHELL04_LEN_BOSS"), 0xa2},
	{__("WP010_SHELL05_LEN_BOSS"), 0xa3},
	{__("WP010_SHELL06_LEN_BOSS"), 0xa4},
	{__("WP010_SHELL07_LEN_BOSS"), 0xa5},
	{__("WP010_SHELL08_LEN_BOSS"), 0xa6},
	{__("WP010_SHELL09_LEN_BOSS"), 0xa7},
	{__("WP003_SHELL10_LEN_BOSS"), 0xa8},
	{__("ATCK_SRV_DRIVE_LEN_BOSS"), 0xa9},
	{__("WP023_SHELL11_LEN_BOSS"), 0xaa},
	{__("RG_BLOCK_ADD_RG_GAUGE"), 0xab},
	{__("RG_BLOCK_ADD_RG_GAUGE_JUST"), 0xac},
	{__("RG_BLOCK_ADD_RG_GAUGE_CRUSH"), 0xad},
	{__("RG_BLOCK_COMBO_UP_BONUS"), 0xae},
	{__("RG_BLOCK_COMBO_UP_BONUS_JUST"), 0xaf},
	{__("RG_RELEASE_COMBO_UP_BONUS_JUST"), 0xb0},
	{__("RG_RELEASE_COMBO_UP_BONUS_GUARD"), 0xb1},
	{__("RG_BLOCK_COMBO_UP_BONUS_TIMER"), 0xb2},
	{__("RG_BLOCK_COMBO_UP_BONUS_JUST_TIMER"), 0xb3},
	{__("RG_RELEASE_COMBO_UP_BONUS_JUST_TIMER"), 0xb4},
	{__("RG_RELEASE_COMBO_UP_BONUS_GUARD_TIMER"), 0xb5},
	{__("RG_ADD_RG_GAUGE_PARAM_A"), 0xb6},
	{__("RG_ADD_RG_GAUGE_PARAM_B"), 0xb7},
	{__("RG_ADD_RG_GAUGE_BONUS_JUST"), 0xb8},
	{__("RG_ADD_RG_GAUGE_BONUS_GUARD"), 0xb9},
	{__("RG_BLOCK_KNOCK_BACK_SPD_XZ"), 0xba},
	{__("RG_BLOCK_KNOCK_BACK_ADD_XZ"), 0xbb},
	{__("RG_BLOCK_GUARD_KNOCK_BACK_SPD_XZ"), 0xbc},
	{__("RG_BLOCK_GUARD_KNOCK_BACK_ADD_XZ"), 0xbd},
	{__("RG_BLOCK_AIR_KNOCK_BACK_SPD_XZ"), 0xbe},
	{__("RG_BLOCK_AIR_KNOCK_BACK_ADD_XZ"), 0xbf},
	{__("RG_BLOCK_GUARD_AIR_KNOCK_BACK_SPD_XZ"), 0xc0},
	{__("RG_BLOCK_GUARD_AIR_KNOCK_BACK_ADD_XZ"), 0xc1},
	{__("ATCK_LF_AIR_COMBO_SPD_Y"), 0xc2},
	{__("ATCK_LF_AIR_COMBO_ADD_Y"), 0xc3},
	{__("ATCK_LF_AIR_COMBO_SPD2_Y"), 0xc4},
	{__("ATCK_LF_AIR_COMBO_ADD2_Y"), 0xc5},
	{__("ATCK_AIR_CNT_LF_COMBO"), 0xc6},
	{__("ATCK_GM_SAVE_TIME_LV1"), 0xc7},
	{__("ATCK_GM_SAVE_TIME_LV2"), 0xc8},
	{__("SGM_REAL_IMPACT_SPD_Y"), 0xc9},
	{__("SGM_REAL_IMPACT_ADD_Y"), 0xca},
	{__("SGM_INFERNO_SPD_Y"), 0xcb},
	{__("SGM_INFERNO_ADD_Y"), 0xcc},
	{__("ATCK_AIR_CNT_SGM_INFERNO"), 0xcd},
	{__("ATCK_PD_SHOT_SAVE_TIME_LV1"), 0xce},
	{__("ATCK_PD_SHOT_SAVE_TIME_LV2"), 0xcf},
	{__("PD_GUAGE_MAX_LV0"), 0xd0},
	{__("PD_GUAGE_MAX_LV1"), 0xd1},
	{__("PD_GUAGE_MAX_LV2"), 0xd2},
	{__("PD_GUAGE_MAX_LV3"), 0xd3},
	{__("ATCK_PD_METEOR_GUAGE_FIX_TIME"), 0xd4},
	{__("WP023_SHELL14_SPEED"), 0xd5},
	{__("WP023_SHELL14_LEN"), 0xd6},
	{__("WP023_SHELL14_LEN_BOSS"), 0xd7},
	{__("WP023_SHELL15_SPEED"), 0xd8},
	{__("WP023_SHELL15_LEN"), 0xd9},
	{__("WP023_SHELL15_LEN_BOSS"), 0xda},
	{__("WP023_SHELL16_SPEED"), 0xdb},
	{__("WP023_SHELL16_LEN"), 0xdc},
	{__("WP023_SHELL16_LEN_BOSS"), 0xdd},
	{__("WP023_SHELL17_SPEED"), 0xde},
	{__("WP023_SHELL17_LEN"), 0xdf},
	{__("WP023_SHELL17_LEN_BOSS"), 0xe0},
	{__("ATCK_PD_METEOR_SHOT_TIME"), 0xe1},
	{__("GM_KICK_13R_DVL_SPD_Y"), 0xe2},
	{__("GM_KICK_13R_DVL_ADD_Y"), 0xe3},
	{__("ATCK_AIR_CNT_MGUN_REPEAT"), 0xe4},
	{__("DANTE_DEVIL_TRG_DOWN_VAL"), 0xe5},
	{__("ATCK_PD_MGUN_SHOT_TIME"), 0xe6},
	{__("PD_GUAGE_DOWN_METEOR"), 0xe7},
	{__("PD_GUAGE_DOWN_METEOR_SHOT"), 0xe8},
	{__("PD_GUAGE_DOWN_METEOR_SHOT2"), 0xe9},
	{__("PD_GUAGE_DOWN_BOOMERANG"), 0xea},
	{__("ATCK_PD_BOOMERANG_GUAGE_FIX_TIME"), 0xeb},
	{__("ATCK_PD_BOOMERANG_HIT_TIME"), 0xec},
	{__("ATCK_PD_BOOMERANG_HIT_ENABLE_TIME"), 0xed},
	{__("ATCK_SRV_DRIVE_UP_TIME_LV1"), 0xee},
	{__("ATCK_SRV_DRIVE_UP_TIME_LV2"), 0xef},
	{__("PD_GUAGE_UP_LAUNCHER"), 0xf0},
	{__("PD_GUAGE_UP_LAUNCHER2"), 0xf1},
	{__("PD_GUAGE_UP_LAUNCHER3"), 0xf2},
	{__("PD_GUAGE_UP_MACHINE_GUN"), 0xf3},
	{__("WP003_SHELL03_TARGET_INNER"), 0xf4},
	{__("WP003_SHELL10_TARGET_INNER"), 0xf5},
	{__("WP023_SHELL15_TARGET_INNER"), 0xf6},
	{__("WP010_SHELL05_TARGET_INNER"), 0xf7},
	{__("ATCK_PD_METEOR_S_SPD_Y"), 0xf8},
	{__("ATCK_PD_METEOR_S_ADD_Y"), 0xf9},
	{__("ATCK_PD_METEOR_E_SPD_Y"), 0xfa},
	{__("ATCK_PD_METEOR_E_ADD_Y"), 0xfb},
	{__("RG_INVINCIBLE_FIX_TIME"), 0xfc},
	{__("RG_INVINCIBLE_GUAGE_DOWN"), 0xfd},
	{__("RG_INVINCIBLE_END_TIME"), 0xfe},
	{__("RG_INVINCIBLE_HITSTOP_TIME00"), 0xff},
	{__("RG_INVINCIBLE_JOINT_VIB_UP00"), 0x100},
	{__("RG_INVINCIBLE_JOINT_VIB_DOWN00"), 0x101},
	{__("RG_INVINCIBLE_JOINT_VIB_FRAME00"), 0x102},
	{__("RG_INVINCIBLE_HITSTOP_TIME01"), 0x103},
	{__("RG_INVINCIBLE_JOINT_VIB_UP01"), 0x104},
	{__("RG_INVINCIBLE_JOINT_VIB_DOWN01"), 0x105},
	{__("RG_INVINCIBLE_JOINT_VIB_FRAME01"), 0x106},
	{__("TS_AIR_ESC_KNEEL_FRAME_S"), 0x107},
	{__("TS_AIR_ESC_KNEEL_FRAME_Y"), 0x108},
	{__("TS_AIR_ESC_KNEEL_FRAME_E"), 0x109},
	{__("TS_AIR_ESC_COMBO_FRAME_S"), 0x10a},
	{__("TS_AIR_ESC_COMBO_FRAME_Y"), 0x10b},
	{__("TS_AIR_ESC_COMBO_FRAME_E"), 0x10c},
	{__("TS_AIR_ESC_HITIME_FRAME_S"), 0x10d},
	{__("TS_AIR_ESC_HITIME_FRAME_Y"), 0x10e},
	{__("TS_AIR_ESC_HITIME_FRAME_E"), 0x10f},
	{__("TS_AIR_ESC_COMBO_UP_BONUS"), 0x110},
	{__("TS_AIR_ESC_COMBO_UP_BONUS_TIME"), 0x111},
	{__("ATCK_AIR_CNT_PD_METEOR"), 0x112},
	{__("ATCK_YT_SPACE_EFC_X"), 0x113},
	{__("ATCK_YT_SPACE_EFC_Y"), 0x114},
	{__("ATCK_YT_SPACE_EFC_Z"), 0x115},
	{__("ATCK_LF_SHL_REARRANGEMENT_TIME"), 0x116},
	{__("ATCK_LF_SHL_WAIT_BURST_TIME"), 0x117},
	{__("ATCK_LF_SHL_DETONATE_TIME"), 0x118},
	{__("ATCK_LF_SHL_DETONATE_INTERVAL_TIME"), 0x119},
	{__("ATCK_LF_SHL_INTENSIVE_TIME"), 0x11a},
	{__("ATCK_LF_SHL_INTENSIVE_INTERVA_TIME"), 0x11b},
	{__("ATCK_LF_SHL_INTENSIVE_SPD"), 0x11c},
	{__("ATCK_LF_SHL_EM_CNST_BURST_TIME"), 0x11d},
	{__("ATCK_YT_AIR_COMBO0_SPD_Y"), 0x11e},
	{__("ATCK_YT_AIR_COMBO0_ADD_Y"), 0x11f},
	{__("ATCK_AIR_CNT_YT_COMBO0"), 0x120},
	{__("ATCK_SCA_NUNCHAKUS_ID_PROBABILITY"), 0x121},
	{__("ATCK_YT_AIR_COMBO1_SPD_Y"), 0x122},
	{__("ATCK_YT_AIR_COMBO1_ADD_Y"), 0x123},
	{__("ATCK_AIR_CNT_YT_COMBO1"), 0x124},
	{__("ATCK_LF_AIR_DETONATE_ALL_SPD_Y"), 0x125},
	{__("ATCK_LF_AIR_DETONATE_ALL_ADD_Y"), 0x126},
	{__("ATCK_AIR_CNT_LF_DETONATE_ALL"), 0x127},
	{__("ATCK_YT_COMBO2_COMBO_UP_BONUS"), 0x128},
	{__("ATCK_YT_COMBO2_COMBO_UP_BONUS_TIMER"), 0x129},
	{__("ATCK_YT_COMBO2_DT_UP_BONUS"), 0x12a},
	{__("ATCK_YT_SPACE2_EFC_X"), 0x12b},
	{__("ATCK_YT_SPACE2_EFC_Y"), 0x12c},
	{__("ATCK_YT_SPACE2_EFC_Z"), 0x12d},
	{__("ATCK_LF_CROSS_THROW_TIME00"), 0x12e},
	{__("ATCK_LF_CROSS_THROW_TIME01"), 0x12f},
	{__("ATCK_LF_AIR_COMBO_THROW_TIME00"), 0x130},
	{__("ATCK_LF_AIR_COMBO_THROW_TIME01"), 0x131},
	{__("ATCK_LF_AIR_COMBO_THROW_TIME02"), 0x132},
	{__("DT_HP_RECOVER"), 0x133},
	{__("ATCK_LF_COMBO0_TIME00"), 0x134},
	{__("ATCK_LF_COMBO2_TIME00"), 0x135},
	{__("ATCK_LF_COMBO2_TIME01"), 0x136},
	{__("ATCK_LF_DIO_KNIFE_TIME00"), 0x137},
	{__("ATCK_LF_DIO_KNIFE_TIME01"), 0x138},
	{__("ATCK_LF_DIO_KNIFE_TIME02"), 0x139},
	{__("ATCK_LF_DT_DIO_KNIFE_TIME00"), 0x13a},
	{__("ATCK_LF_DT_DIO_KNIFE_TIME01"), 0x13b},
	{__("ATCK_LF_DT_DIO_KNIFE_TIME02"), 0x13c},
	{__("ATCK_LF_SHL_BOM_TIME"), 0x13d},
	{__("ATCK_LF_SHL_BOM_INTERVAL_TIME"), 0x13e},
	{__("DANTE_DEVIL_TRG_DOWN_VAL2"), 0x13f},
	{__("ATCK_SGM_INFERNO_HEIGHT_LIMIT"), 0x140},
	{__("ATCK_DS_AIR_COMBO0_HEIGHT_LIMIT"), 0x141},
	{__("ATCK_DS_AIR_COMBO1_HEIGHT_LIMIT"), 0x142},
	{__("ATCK_LF_BOM_BARRIER_SPD_Y"), 0x143},
	{__("ATCK_LF_BOM_BARRIER_ADD_Y"), 0x144},
	{__("ATCK_AIR_CNT_LF_BOM_BARRIER"), 0x145},
	{__("DT_GAUGE_KEEP_FRM"), 0x146},
	{__("RG_BLOCK_DEVIL_UP_BONUS"), 0x147},
	{__("RG_BLOCK_DEVIL_UP_BONUS_JUST"), 0x148},
	{__("RG_RELEASE_DEVIL_UP_BONUS_JUST"), 0x149},
	{__("RG_RELEASE_DEVIL_UP_BONUS_GUARD"), 0x14a},
	{__("TS_AIR_ESC_DEVIL_UP_BONUS"), 0x14b},
	{__("GM_JUST_SAVE_COMBO_UP_BONUS_TIME"), 0x14c},
	{__("GM_JUST_SAVE_COMBO_UP_BONUS"), 0x14d},
	{__("GM_JUST_SAVE_DEVIL_UP_BONUS"), 0x14e},
	{__("PD_BOX_LV0_COMBO_UP_BONUS_TIME"), 0x14f},
	{__("PD_BOX_LV0_COMBO_UP_BONUS"), 0x150},
	{__("PD_BOX_LV0_DEVIL_UP_BONUS"), 0x151},
	{__("PD_BOX_LV1_COMBO_UP_BONUS_TIME"), 0x152},
	{__("PD_BOX_LV1_COMBO_UP_BONUS"), 0x153},
	{__("PD_BOX_LV1_DEVIL_UP_BONUS"), 0x154},
	{__("PD_BOX_LV2_COMBO_UP_BONUS_TIME"), 0x155},
	{__("PD_BOX_LV2_COMBO_UP_BONUS"), 0x156},
	{__("PD_BOX_LV2_DEVIL_UP_BONUS"), 0x157},
	{__("PD_BOX_LV3_COMBO_UP_BONUS_TIME"), 0x158},
	{__("PD_BOX_LV3_COMBO_UP_BONUS"), 0x159},
	{__("PD_BOX_LV3_DEVIL_UP_BONUS"), 0x15a},
	{__("DT_GUAGE_DOWN_STINGER"), 0x15b},
	{__("DT_GUAGE_DOWN_KICK13"), 0x15c},
	{__("DT_GUAGE_DOWN_DIO_KNIFE"), 0x15d},
};


template<typename ParamArray, size_t N>
void DisplayParameters(void* baseAddress, const ParamArray (&params)[N], const char* windowTitle) {
    float* floatArray = (float*)baseAddress;
    static std::unordered_map<std::string, std::vector<float>> backupMap;
    static std::unordered_map<std::string, bool> hasBackupMap;
    
    std::string key(windowTitle);
    auto& backupValues = backupMap[key];
    auto& hasBackup = hasBackupMap[key];
    
    ImGui::Begin(windowTitle, &PlayerTracker::show_player_params);
	ImGui::UpdateCurrentFontSize(0.8f * ImGui::GetStyle().FontSizeBase);
    
    if (ImGui::Button(_("Save Current Values"))) {
        backupValues.clear();
        backupValues.reserve(N);
        for (const auto& param : params) {
            backupValues.push_back(floatArray[param.offset]);
        }
        hasBackup = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button(_("Apply Saved Values"))) {
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
	ImGui::UpdateCurrentFontSize(0.8f * ImGui::GetStyle().FontSizeBase);
    ImGui::PushItemWidth(100);
    for (size_t i = 0; i < N; ++i) {
        float& value = floatArray[params[i].offset];
        ImGui::InputFloat(utility::text_lookup((char*)params[i].name), &value);
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
			DisplayParameters(player->charactersettingsThree, danteParams, _("Dante Parameters"));
		}
		else {
			DisplayParameters(player->charactersettingsThree, neroParams, _("Nero Parameters"));
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

const char* moveLineNames[]{
	__("MoveLine 0:"),
	__("MoveLine 1:"),
	__("MoveLine 2:"),
	__("MoveLine 3:"),
	__("MoveLine 4:"),
	__("MoveLine 5:"),
	__("MoveLine 6:"),
	__("MoveLine 7:"),
	__("MoveLine 8:"),
	__("MoveLine 9:"),
	__("MoveLine 10:"),
	__("MoveLine 11:"),
	__("MoveLine 12:"),
	__("MoveLine 13 (Players):"),
	__("MoveLine 14"),
	__("MoveLine 15 (Enemies):"),
	__("MoveLine 16:"),
	__("MoveLine 17:"),
	__("MoveLine 18:"),
	__("MoveLine 19:"),
	__("MoveLine 20:"),
	__("MoveLine 21:"),
	__("MoveLine 22:"),
	__("MoveLine 23:"),
	__("MoveLine 24:"),
	__("MoveLine 25:"),
	__("MoveLine 26:"),
	__("MoveLine 27:"),
	__("MoveLine 28:"),
	__("MoveLine 29:"),
	__("MoveLine 30:"),
	__("MoveLine 31:"),
};

void PlayerTracker::on_gui_frame(int display) {
	ImGui::Checkbox(_("Show Player Params"), &show_player_params);

    if (ImGui::CollapsingHeader(_("Shadow Settings"))) {
        sUnit* sUnit = devil4_sdk::get_sUnit();
        if (sUnit) {
			uShadow* shadow = (uShadow*)sUnit->mMoveLine[1].mBottom;
            if (shadow) {
                ImGui::Indent(lineIndent);
                ImGui::InputFloat3(_("Rotation"), &shadow->rotation.x);
                ImGui::InputFloat4(_("Colour"), &shadow->colour.x);
                ImGui::InputFloat(_("diffuse"), &shadow->diffuse);
                ImGui::InputFloat(_("bloom"), &shadow->bloom);
                ImGui::Unindent();
            }
        }
    }

    if (ImGui::CollapsingHeader(_("View MoveLine Entries"))) {
        sUnit* sUnit = devil4_sdk::get_sUnit();
        if (sUnit) {
			for (int i = 0; i < 32; i++) {
				cUnit* mL = (cUnit*)sUnit->mMoveLine[i].mTop;
				int mLCount = 0;
				ImGui::Text(moveLineNames[i]);
				while (mL) {
					ImGui::PushID(mL);
					ImGui::Indent(lineIndent);
					ImGui::PushItemWidth(sameLineItemWidth);
					char entryText[16];
					snprintf(entryText, sizeof(entryText), "entry %i", mLCount);
					ImGui::InputScalar(entryText, ImGuiDataType_S32, &mL, NULL, NULL, "%8X", ImGuiInputTextFlags_CharsHexadecimal);
					mL = mL->mp_next_unit;
					mLCount++;
					ImGui::PopItemWidth();
					ImGui::Unindent(lineIndent);
					ImGui::PopID();
				}
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
            if (ImGui::Button(_("Play Animation ID"))) {
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
