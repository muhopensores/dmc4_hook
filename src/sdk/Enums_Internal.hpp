#include <cstdint>

enum AllocatorEnum {
	DEFAULT,
	GLOBAL,
	TEMP,
	SYSTEM,
	UNIT,
	AREA,
	TRANS,
	ARRAY,
	SOUND
};

enum ACTOR_STATUS {
    AS_DEFAULT  = 0,
    AS_GROUND   = 1,
    AS_AIR      = 2,
    AS_DOWN     = 4,
    AS_DAMAGE   = 8,
    AS_GUARD    = 16,
    AS_PARRY    = 32,
    AS_GRAB     = 64,
    AS_DEAD     = 128,
    AS_APPEAR   = 256,
    AS_BUSTER   = 512,
    AS_BLOCKING = 1024,
    AS_CUSTOM   = 65536
};

enum ACTOR_TYPE {
    AT_ACTOR       = 0,
    AT_PLAYER      = 1,
    AT_PLWEAPON    = 2,
    AT_ENEMY       = 3,
    AT_SET         = 4,
    AT_SHL         = 5,
    AT_AVOIDED     = 6,
    ACTOR_TYPE_NUM = 7
};

enum SRESOURCE_MODE {
    MODE_BLOCKING = 0x1,
    MODE_ASYNC = 0x2,
    MODE_USECACHE = 0x4,
    MODE_USEGDATA = 0x8,
    MODE_BACKGROUND = 0x20,
    MODE_CREATE = 0x40,
    MODE_NOLOAD = 0x80,
    MODE_STREAM = 0x100,
    MODE_PRELOAD = 0x200,
    MODE_QUALITY_LOWEST = 0x1000,
    MODE_QUALITY_LOW = 0x2000,
    MODE_QUALITY_HIGH = 0x4000,
    MODE_QUALITY_HIGHEST = 0x8000
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
