#pragma once

#include <cstdint>

enum BE_FLAG : std::uint8_t { // TODO: check if uint8_t works
    BEFLAG_DISABBLE,
    BEFLAG_PRE_MOVE,
    BEFLAG_MOVE,
    BEFLAG_PRE_DELETE,
    BEFLAG_DELETE
};

enum UNIT_ATTR : std::uint8_t { // TODO: check if uint8_t works
    UATTR_MOVE = 1,
    UATTR_DRAW = 2,
    UATTR_SELECT = 4,
    UATTR_FIX = 8,
    UATTR_VISIBLE = 16
};

struct cUnit { // NOLINT
    void *vtable_ptr;
    union {
        uint32_t raw;
        struct { //NOLINT
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

class uHasDelta : public cUnit {
public:
    char pad_18[4]; // 0x18
}; // Size: 0x1c
