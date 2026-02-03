#pragma once

#include "cSystem.hpp"
#include <cstdint>

class cUnit;

class MoveLine {
public:
    void* vtable;           // 0x0
    char* mName;            // 0x4
    uint32_t mParallel : 1; // 0x8
    uint32_t mPause : 1;
    uint32_t mTrans : 1;
    uint32_t mLineType : 6;
    uint32_t reserved : 23;
    cUnit* mTop;      // 0xC
    cUnit* mBottom;   // 0x10
    float mDeltaTime; // 0x14
};
static_assert(sizeof(MoveLine) == 0x18);

class sUnit : public cSystem {
public:
    MoveLine mMoveLine[32];
    virtual void reset_();
    virtual void moveExForAll();
    virtual void nullsub_();
    virtual void moveEx(int move_line);
    virtual void moveAfter(int move_line);
    virtual void moveAfter();
    virtual const char* getLineName(int move_line);
    virtual void sync(int move_line);
};
static_assert(sizeof(sUnit) == 0x320);
