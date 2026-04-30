#include <cstdint>

class uMissionStart {
public:
uint32_t vtable; //0x0000
	char pad_0004[16]; //0x0004
	uint8_t flags; //0x0014
	char pad_0015[43]; //0x0015
	uint32_t cursor; //0x0040
    char pad_0044[60]; // 0x0044
}; // Size: 0x0080
static_assert(sizeof(uMissionStart) == 0x80);

class uBloodyPalaceStart {
public:
    uint32_t vtable; //0x0000
	char pad_0004[16]; //0x0004
	uint8_t flags; //0x0014
	char pad_0015[43]; //0x0015
    uint32_t cursor;   // 0x0040
    char pad_0044[60]; // 0x0044
}; // Size: 0x0080
static_assert(sizeof(uMissionStart) == 0x80);
