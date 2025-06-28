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