#pragma once

#include "MtObject.hpp"

static constexpr auto RESOURE_PATH_SIZE = 64;

class cResource : public MtObject
{
public:
    virtual void callDestructor(); // 0x0
    virtual void getMtFileInfo();//0x14
    virtual void getExtension(); //0x18

    using uint32_t = unsigned int;
    using int64_t  = long long;

    char path[RESOURE_PATH_SIZE]; //0x0004
    uint32_t m_ref_count; //0x0044
    uint32_t m_attr; //0x0048
    uint32_t m_flags_something; //0x004C
    uint32_t m_size; //0x0050
    int64_t m_id; //0x0054
    char pad_005_c[4]; //0x005C
}; //Size: 0x0068
static_assert(sizeof(cResource) == 0x60);
