#pragma once

#include "cSystem.hpp"

struct IDirect3DDevice9;

class sRender : public cSystem
{
public:
    char pad_0020[20]; //0x0020
    class ID3DDevice9 *mp_device; //0x0034
    char pad_0038[12]; //0x0038
    Vector2i screenRes; //0x0044
    char pad_004C[88292]; //0x004C
    uint32_t mFrameNum; //0x15930
}; //Size: 0x15934
static_assert(sizeof(sRender) == 0x15934);
static_assert(offsetof(sRender, pad_0020) == 0x0020);
static_assert(offsetof(sRender, mp_device) == 0x0034);
static_assert(offsetof(sRender, pad_0038) == 0x0038);
static_assert(offsetof(sRender, screenRes) == 0x0044);
static_assert(offsetof(sRender, pad_004C) == 0x004C);
static_assert(offsetof(sRender, mFrameNum) == 0x15930);
