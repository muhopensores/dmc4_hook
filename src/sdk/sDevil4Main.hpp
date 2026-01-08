#pragma once

class sDevil4Main {
public:
    char pad_0000[66376];    // 0x0000
    class sRender* s_render; // 0x10348
    char pad_1034_c[60];     // 0x1034C
}; // Size: 0x10388
static_assert(sizeof(sDevil4Main) == 0x10388);

class sDevil4MainPtr {
public:
    class sDevil4Main* mp_devil4_main; // 0x0000
    char pad_0004[60];                 // 0x0004
}; // Size: 0x0040
static_assert(sizeof(sDevil4MainPtr) == 0x40);