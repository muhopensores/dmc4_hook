#include <cstdint>

// Created with ReClass.NET 1.2 by KN4CK3R

class uMissionMenu {
public:
	uint32_t vtable; //0x0000
	char pad_0004[16]; //0x0004
	uint8_t flags; //0x0014
	char pad_0015[43]; //0x0015
	uint32_t cursor; //0x0040
    char pad_0044[40];   // 0x0044
    uint32_t idk;        // 0x006C
    uint32_t hum;        // 0x0070
    uint32_t dvh;        // 0x0074
    uint32_t sos;        // 0x0078
    uint32_t dmd;        // 0x007C
    uint32_t ldk;        // 0x0080
    uint32_t hoh;        // 0x0084
    uint32_t hah;        // 0x0088
    uint32_t character;        // 0x008C
    char pad_008C[1968]; // 0x008C
}; // Size: 0x0840
static_assert(sizeof(uMissionMenu) == 0x840);
