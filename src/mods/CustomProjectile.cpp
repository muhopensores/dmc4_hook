#include "CustomProjectile.hpp"

#define MODEL_PATH "model\\game\\em010\\em010"
#define ATK_PATH ""
#define COL_PATH ""

uintptr_t CustomProjectile::jmp_ret1 { NULL };
uintptr_t CustomProjectile::jmp_ret2{ NULL };
constexpr uintptr_t rModelDTI = 0x00EADF48;
constexpr uintptr_t rAtckStatDTI = 0x00E57498;
constexpr uintptr_t rColShapeDTI = 0x00E575F8;
constexpr uintptr_t rDfdStatDTI = 0x00E57638;
constexpr uintptr_t sDevil4Resource_ptr = 0x00E552D0;
constexpr uintptr_t uActorVtablePtr = 0x00BC4B78;
constexpr size_t uActorVtableSize = 79;
std::unique_ptr<CustomProjectileVtable> CustomVtable;
typedef void(__thiscall* rModelLoad)(void*, void*);

naked void* __cdecl MemberFuncToPtr(...) {
    __asm {
        mov eax, [esp+4]
        ret
    }
}

void __stdcall bring_assert(void* rFile) {
    uintptr_t assert_call = 0x008DDA00;
    _asm {
        mov eax,[sDevil4Resource_ptr]
        mov edi,[rFile]
        call assert_call
    }
}

void __stdcall load_atk_col(void* rAtck, void* rCol, void* ColMgr, void*Obj) {
    uintptr_t load_call = 0x008DF530;
    _asm {
        push [rAtck]
        push [rCol]
        mov esi,[ColMgr]
        mov eax,[Obj]
        call load_call
    }
}

CustomProjectileVtable::CustomProjectileVtable(void* vtable, size_t size) : ProjectileVtable(vtable, size) {
    this->my_vtable[0x14/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::startup_override);
    //this->my_vtable[0x14/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectile::startup_override)
    //this->my_vtable[0x14/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectile::startup_override)
}

void CustomProjectileProp::startup_override() {
    //void* model = devil4_sdk::get_stuff_from_files((void*)rModelDTI, MODEL_PATH, 1);
    //(*(rModelLoad)(&(this->uModelBase.uCoordBase.cUnitBase.vtable_ptr)+0x40))(this, model);
    //if (&model) {
    //    bring_assert(model);
    //}
    ////void* rAtck = devil4_sdk::get_stuff_from_files(rAtckStatDTI, ATK_PATH, 1);
    ////void* rCol = devil4_sdk::get_stuff_from_files(rColShapeDTI, COL_PATH, 1);
}



CustomProjectileProp::CustomProjectileProp(float keepAlive, float force, void* parent = 0, int parentJoint = 0) {
    this->keepAliveTime = keepAliveTime;
    this->force = force;
    this->uModelBase.uCoordBase.vtable_ptr = (void*)parent;
    this->uModelBase.ParentJoint = parentJoint;
    this->uModelBase.vtable_ptr              = (void*)&CustomVtable->my_vtable;
}

void sUnit_spawn_call(void* sUnit, void* obj_to_spawn, int moveline) {
    constexpr uintptr_t fptr_spawn_or_something = 0x008DC540;
    __asm {
			mov eax, [sUnit]
			mov eax, [eax]
			mov esi, obj_to_spawn
			mov ecx, moveline
			push moveline
			call fptr_spawn_or_something
    }
}

void CustomProjectile::SpawnProjectile() {
    void* projptr = devil4_sdk::mt_allocate_heap(sizeof(CustomProjectile),16);
    CustomProjectileProp *proj = new (projptr) CustomProjectileProp(10.0f, 2.0f,(void*)devil4_sdk::get_local_player(),0x96);
    sUnit_spawn_call((void*)0x00E552CC, (void*)proj, 0x12);
}

std::optional<std::string> CustomProjectile::on_initialize() {
    CustomVtable = std::make_unique<CustomProjectileVtable>((void*)uActorVtablePtr, uActorVtableSize);
    return Mod::on_initialize();
}
