#pragma once

#include "cSystem.hpp"
#include "..\MtMath.hpp"

struct IDirect3DDevice9;

class sRender : public cSystem {
public:
    char              pad_0020[20]; // 0x0020
    IDirect3DDevice9* mp_device; // 0x0034
    char              pad_0038[12]; // 0x0038
    Vector2           screenRes; // 0x0044
}; // Size: 0x004C
