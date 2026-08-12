#include "RayTest.hpp"
#include "..\sdk\Devil4.hpp"
#include "..\sdk\sMediator.hpp"
#include "..\sdk\Custom.hpp"
#include "..\sdk\MtDTI.hpp"
#include "..\sdk\MtMath.hpp"

static uintptr_t findIntersection = 0x0095B320;
static uintptr_t submitLine = 0x0045D1F0;
static uintptr_t getCPrim = 0x00A34EE0;
static uint color = 0x00FF00;
static uint thing = 0x20000; //wall

#pragma pack(push, 2)
struct Param {
    int type;
    int filter;
    int hitCallback_this;
    int hitCallback;
    void* hitCallback_data;
    int _pad14;
    int handle_exclude_0;
    int handle_exclude_1;
    int _pad20;
    int correctionNum;
    unsigned __int8 bAdjMvRelSet;
    unsigned __int8 _pad29;
    unsigned __int8 bAdjAntiStop;
    unsigned __int8 _pad2B;
    float capsuleFwdLimit;
    float capsuleBwdLimit;
    float sweepRadiusCap;
    int partsFilter;
    int _pad3C;
    char pad[0x40];
};
#pragma pack(pop)
//static_assert(sizeof(Param) == 0x40);

static bool __stdcall findIntersection_wr(Param* param, void* sCol, MtLineSegment* ls, bool both_side, MtVector3* out) {
    bool res = false;
    _asm {
        push ebx
        mov ebx,esp
        pushad
        mov eax,param
        push out
        push both_side
        push ls
        mov edi, [sCol]
        mov edx,[edi]
        push edx
        call findIntersection
        mov byte ptr res, al
        popad
        mov esp,ebx
        pop ebx
    }
    return res;
}

static void submitLine_wr(void* cprim, MtVector3* start, MtVector3* end, unsigned int color, uint* flags) {
    _asm {
        pushad
        mov ecx,cprim
        mov edi,start
        push flags
        push color
        push end
        call submitLine
        popad
    }
}

static void* getCPrim_wr(void* pTrans, void* sPrim, uint prim_type, cUnit* p_unit) {
    void* res = nullptr;
    __asm {
        pushad
        mov esi,pTrans
        push ecx
        push prim_type
        mov ecx,[sPrim]
        mov eax, [ecx]
        push eax
        call getCPrim
        mov res,eax
        popad
    }
    return res;
}

struct CollLine : CustomActor {
    MtLineSegment ls;
    uint color = 0xFF0000FF;
    void* cprim = nullptr;
    CollLine();
    MtDTI* getDTI();
    void startup_override() {};
    void render(void* ptrans);
    void lifecycle_override();
};
REGISTER_VTABLE(CollLine);

void CollLine::render(void* ptrans) {
    sMediator* smed = devil4_sdk::get_sMediator();
    uPlayer* pl      = smed->player_ptr;
    MtVector3 out;
    MtVector3 pl_now_pos = *(MtVector3*)(uintptr_t(pl) + 0x1350);
    MtVector3 ray_end    = pl_now_pos;
    ray_end.z += 100.0f;
    void* sPrim = (void*)0x00E559D4;
    void* cprim     = getCPrim_wr(ptrans, sPrim, 6, pl);
    uint flags[2]{ 0xffffffff , 16};
    submitLine_wr(cprim, &pl_now_pos, &ray_end, this->color, flags);
}

void CollLine::lifecycle_override() {
    sMediator* smed = devil4_sdk::get_sMediator();
    uPlayer* pl      = smed->player_ptr;
    if (pl) {
        Param param;
        param.partsFilter      = 0;
        param.capsuleFwdLimit  = 3.4028235e38;
        param.capsuleBwdLimit  = 3.4028235e38;
        param.sweepRadiusCap   = 0.0f;
        param.handle_exclude_0 = -1;
        param.handle_exclude_1 = -1;
        param._pad20           = 0;
        param.correctionNum    = 10;
        param.bAdjMvRelSet     = 1;
        param._pad29           = 0;
        param.bAdjAntiStop     = 1;
        param._pad2B           = 0;
        param.partsFilter      = 0;
        param.type             = 127;
        param.filter           = thing;
        memset(&param.hitCallback_this, 0, 12);

        char out[0x1000]; //stack fuck-up around here, todo
        MtVector3 pl_now_pos = *(MtVector3*)(uintptr_t(pl) + 0x1350);
        MtVector3 ray_end    = pl_now_pos;
        ray_end.z += 100.0f;
        this->ls.p0 = pl_now_pos;
        this->ls.p1 = ray_end;
        uintptr_t sCol = 0x00E559D0;
        CollLine* thisPtr = this;
        if (findIntersection_wr(&param, (void*)sCol, &this->ls, 0, (MtVector3*)&out))
            thisPtr->color = 0x00FF00;
        else
            thisPtr->color = 0xFF0000;
    }
}

static void sUnit_spawn_call(void* sUnit, void* obj_to_spawn, int moveline) {
    constexpr uintptr_t fptr_spawn_or_something = 0x008DC540;
    __asm {
			mov eax, [sUnit]
			mov eax, [eax]
			mov esi, obj_to_spawn
			push moveline
			call fptr_spawn_or_something
    }
}

std::unique_ptr<CollLineVtable> CustomVtable;
static class MtDTI CustomDTI;

MtDTI* CollLine::getDTI() { return &CustomDTI; }
CollLine::CollLine() {
    custom_utils::uActorCons(this);
    this->vtable_ptr = (uintptr_t*)CustomVtable->my_vtable.data();
}

void SpawnCollLine() {
    void* projptr = devil4_sdk::mt_allocate_heap(sizeof(CollLine), 16);
    // void* projptr              = devil4_sdk::mt_allocate_heap(0x18D0, 16);
    CollLine* proj = new (projptr) CollLine();
    sUnit_spawn_call((void*)0x00E552CC, (MtObject*)proj, 12);
}

void RayTest::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Button("Coll Test")) {
            // if (fileExists)
            SpawnCollLine();
        }
        ImGui::InputInt("Filter", (int*)&thing);
        // ImGui::SameLine();
        // help_marker(_("I put a file check here so if this suddenly stopped working blame me")); // mf
    }
}

std::optional<std::string> RayTest::on_initialize() {
    CustomDTI.m_size    = sizeof(CollLine);
    CustomDTI.m_name    = "CollLine";
    CustomDTI.mp_child  = 0;
    CustomDTI.mp_link   = 0;
    CustomDTI.mp_next   = 0;
    CustomDTI.mp_parent = (MtDTI*)0xE5B310;
    CustomDTI.m_id      = 7;

    CustomVtable = std::make_unique<CollLineVtable>((void*)uActorVtablePtr, uActorVtableSize);

    return Mod::on_initialize();
}