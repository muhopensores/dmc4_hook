#pragma once

#include "cSystem.hpp"

struct IDirect3DDevice9;

class sRender : public cSystem {
public:
    char pad_0024[16]; //0x0024
    IDirect3DDevice9* mp_device; // 0x0034
    char pad_0038[12]; //0x0038
    Vector2i          screenRes; // 0x0044
    char pad_004C[88292]; //0x004C
    uint32_t          mFrameNum; //0x15930
}; //Size: 0x15934
static_assert(sizeof(sRender) == 0x15930);

