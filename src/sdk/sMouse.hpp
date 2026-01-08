#pragma once

class sMouse {
public:
    char pad_0000[144];       // 0x0000
    bool m_show_mouse_cursor; // 0x0090
    bool N00000272;           // 0x0091
    bool N00000276;           // 0x0092
    char pad_0093[2029];      // 0x0093
}; // Size: 0x0880
static_assert(sizeof(sMouse) == 0x880);

class sMousePtr {
public:
    class sMouse* m_mouse_ptr; // 0x0000
}; // Size: 0x0004
static_assert(sizeof(sMousePtr) == 0x4);
