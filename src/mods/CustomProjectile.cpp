#include "CustomProjectile.hpp"
#include <cmath>

uintptr_t CustomProjectile::jmp_ret1 { NULL };
uintptr_t CustomProjectile::jmp_ret2{ NULL };
constexpr uintptr_t rModelDTI = 0x00EADF48;
constexpr uintptr_t rAtckStatDTI = 0x00E57498;
constexpr uintptr_t rColShapeDTI = 0x00E575F8;
constexpr uintptr_t rDfdStatDTI = 0x00E57638;
constexpr uintptr_t sDevil4Resource_ptr = 0x00E552D0;
constexpr uintptr_t uActorVtablePtr = 0x00BC4B78; //uActor
constexpr uintptr_t JavelinVtablePtr = 0x0BD4270;//uEmShl022Javelin
constexpr size_t uActorVtableSize = 79;
char* MODEL_PATH = "model\\game\\wp023\\wp023_06";
char* ATK_PATH = "Collision\\vieris";
char* COL_PATH = "Collision\\vieris";
char* EFL_PATH = "effect\\efl\\ene\\ee006_40v0";
std::unique_ptr<CustomProjectileVtable> CustomVtable;
class MtDTI CustomDTI;
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
        mov eax,[eax]
        mov edi,[rFile]
        call assert_call
    }
}

void __stdcall load_atk_col(void* rAtck, void* rCol, void* ColMgr, void*Obj) {
    uintptr_t load_call = 0x0050BDD0;
    _asm {
        pushad
        push [rAtck]
        push [rCol]
        mov esi,[ColMgr]
        mov eax,[Obj]
        call load_call
        popad
    }
}

void __stdcall get_model(void* obj, void* model) {
    _asm {
        pushad
        mov ecx,[obj]
        mov eax,[ecx]
        mov edx,[eax+0x40]
        mov eax,[model]
        push eax
        call edx
        popad
    }
}

void __stdcall updateLmat(void* obj) {
    _asm {
        pushad
        mov ecx,[obj]
        mov eax,[ecx]
        mov edx,[eax+0x34]
        call edx
        popad
    }
}

void __stdcall updateWmat(void* obj) {
    _asm {
        pushad
        mov ecx,[obj]
        mov eax,[ecx]
        mov edx,[eax+0x38]
        call edx
        popad
    }
}

void __stdcall despawn(void* obj) {
    _asm {
        mov esi,[obj]
        mov eax,[esi+4]
        and eax,-5
        or eax,3
        mov [esi+4],eax
    }
}

void __stdcall ushell_des(void* obj) {
    uintptr_t ushellDTI = 0x00BEE7C0;
    uintptr_t ushell_destructor = 0x004A7280;
    _asm {
        pushad
        mov ecx,[obj]
        mov esi,ecx
        mov edx, ushellDTI
        mov [esi],edx
        call ushell_destructor
        popad
    }
}

void __stdcall uActorCons(void* obj) {
    uintptr_t uActor_constructor = 0x004A6E80;
    _asm {
        mov esi,[obj]
        call uActor_constructor
    }
}

void __stdcall uCollisionMgrCons(void* obj) {
    uintptr_t uCollisionMgr_constructor = 0x0050B080;
    _asm {
            mov ecx,[obj]
            call uCollisionMgr_constructor
    }
}

void __stdcall collide(void* CollMgr) {
    _asm {
            pushad
            mov ecx,[CollMgr]
            mov eax,[ecx]
            mov edx,[eax+0x18]
            call edx
            popad
    }
}

void __stdcall hitbox_call(void* CollMgr, int id) {
    uintptr_t hitbox_call_func = 0x0050CA60;
    float timer = 5.0f;
    _asm {
            pushad
            push 00
            push [CollMgr]
            or edx,-1
            mov eax,[id]
            call hitbox_call_func
            test eax,eax
            je return_addr
            movss xmm0,[timer]
            movss [eax+0x14C],xmm0
            xorps xmm0,xmm0
            movss [eax+0x148],xmm0
            mov esi,[CollMgr]
            mov esi,[esi+0xD8]
            lea edx,[esi+0xA0]
            mov [eax+0x140],edx
            mov byte ptr [eax+0x64],1
        return_addr:
            popad
    }
}

CustomProjectileVtable::CustomProjectileVtable(void* vtable, size_t size) {
    this->size                = size;
    this->my_vtable = std::vector<uintptr_t>((uintptr_t*)vtable, (uintptr_t*)vtable + size);
    //memcpy(this->my_vtable.data() + 0x24, (void*)(JavelinVtablePtr + 0x24), 4);
    this->my_vtable[0] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::destructor);
    this->my_vtable[0x10/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::getDTI);
    this->my_vtable[0x14/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::startup_override);
    this->my_vtable[0x24 / 4] = *(uintptr_t*)(JavelinVtablePtr + 0x24);
    this->my_vtable[0x30/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::die);
    this->my_vtable[0x78/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::lifecycle_override);
    this->my_vtable[0x94/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectileProp::onhit_override);
    //this->my_vtable[0x14/4] = (uintptr_t)MemberFuncToPtr(&CustomProjectile::startup_override)
}

void CustomProjectileProp::render(void* obj) {
    uintptr_t render_jmp = 0x00522DC0;
    _asm {
        mov ecx,[obj]
        jmp render_jmp
    }
}

void CustomProjectileProp::destructor(uint32_t flag) {
    ushell_des(this);
    if (flag) {
        devil4_sdk::unit_deallocate((MtObject*)this);
    }
}

void CustomProjectileProp::die() {
    despawn(this);
}

void CustomProjectileProp::startup_override() {
    //Get model
    void* model = devil4_sdk::get_stuff_from_files((MtDTI*)(uintptr_t)rModelDTI, MODEL_PATH, 1);
    get_model(this, model);
    if (&model) {
        bring_assert(model);
    }

    void* atk_file = devil4_sdk::get_stuff_from_files((MtDTI*)(uintptr_t)rAtckStatDTI, ATK_PATH, 1);
    void* col_file = devil4_sdk::get_stuff_from_files((MtDTI*)(uintptr_t)rColShapeDTI, COL_PATH, 1);
    uActorMain::uCoord* ThisCoord = &this->actor.uActorBase.uModelBase.uCoordBase;
    void* efx = devil4_sdk::effect_generator(EFL_PATH, this, 0x14);


    load_atk_col(atk_file, col_file, &this->CollMgr, this);
    if (&atk_file)
        bring_assert(atk_file);
    if (&col_file)
        bring_assert(col_file);

    hitbox_call(&this->CollMgr, 0);
    ////void* rAtck = devil4_sdk::get_stuff_from_files(rAtckStatDTI, ATK_PATH, 1);
    ////void* rCol = devil4_sdk::get_stuff_from_files(rColShapeDTI, COL_PATH, 1);
}

void CustomProjectileProp::lifecycle_override() {
    updateLmat(this);
    updateWmat(this);
    float tick = this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.m_delta_time;
    this->keepAliveTime += tick;
    if (this->keepAliveTime > this->KeepAliveTimer)
        this->die();
    if (this->HitTimer > 5.0f) {
        hitbox_call(&this->CollMgr, 0);
        collide(&this->CollMgr);
        this->HitTimer = 0;
    }
    else
        this->HitTimer += tick;
    Matrix4x4 Wmat = *(Matrix4x4*)&this->actor.uActorBase.uModelBase.uCoordBase.mWmat;
    Matrix4x4 translate = glm::translate(Wmat, glm::vec3(0,0,tick*10.0f));
    Vector3f* this_coord = (Vector3f*)&this->actor.uActorBase.uModelBase.uCoordBase.mPos;
    this_coord->x = translate[3].x;
    this_coord->y = translate[3].y;
    this_coord->z = translate[3].z;
    return;
}

void CustomProjectileProp::onhit_override(void* atk_param, void* dfd_param) {
    uCollisionMgr* target_collmgr = (uCollisionMgr*)*(uintptr_t*)((uintptr_t)dfd_param+0x98);
    uActor_t* target = target_collmgr->mpReportActor;
    if (!target)
        return;
    Vector3f glmThisVec = *(Vector3f*)&this->actor.uActorBase.uModelBase.uCoordBase.mWmat.vectors[3];
    Vector3f glmTargetVec = *(Vector3f*)&target->uActorBase.uModelBase.uCoordBase.mPos;
    Vector3f forceVec = glm::normalize(glmThisVec - glmTargetVec);
    float dist = glm::distance(glmThisVec, glmTargetVec);
    float TrueForce = std::clamp(this->force * pow(dist * 0.007f + 0.70f, -1.0f), 15.0f, this->force * 1.3f);
    Vector3f* this_coord  = (Vector3f*)&this->actor.uActorBase.uModelBase.uCoordBase.mPos;
    target->mVel.x = TrueForce * forceVec.x;
    target->mVel.z = TrueForce * forceVec.z;
    target->mVel.y = TrueForce * forceVec.y;
    //target->uActorBase.uModelBase.uCoordBase.cUnitBase.m_delta_time = 0.1f;
}

MtDTI* CustomProjectileProp::getDTI() {
    return &CustomDTI;
}


CustomProjectileProp::CustomProjectileProp(float keepAlive, float force, uActorMain::uCoord* parent = 0, int parentJoint = 0) {
    uActorCons(this);
    this->KeepAliveTimer = keepAlive;
    this->HitTimer = 0.0f;
    this->keepAliveTime = 0.0f;
    this->force = force;
    memset(&this->CollMgr, 0, sizeof(uCollisionMgr));
    uCollisionMgrCons(&this->CollMgr);
    //this->actor.uActorBase.uModelBase.uCoordBase.mParent = (uintptr_t)parent;
    //this->actor.uActorBase.uModelBase.uCoordBase.ParentJoint = parentJoint;
    this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.vtable_ptr = (uintptr_t*)CustomVtable->my_vtable.data();
    this->actor.mActorType=5;
    this->actor.uActorBase.mWorkRate.mType=0x9;
    this->actor.mVel.x = 0.0f;
    this->actor.mVel.y = 0.0f;
    this->actor.mVel.z = 0.0f;
    this->actor.uActorBase.uModelBase.uCoordBase.mPos.x = parent->mPos.x;
    this->actor.uActorBase.uModelBase.uCoordBase.mPos.y = parent->mPos.y+500.0f;
    this->actor.uActorBase.uModelBase.uCoordBase.mPos.z = parent->mPos.z;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.x = parent->mQuat.x;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.y = parent->mQuat.y;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.z = parent->mQuat.z;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.w = parent->mQuat.w;
}

void sUnit_spawn_call(void* sUnit, void* obj_to_spawn, int moveline) {
    constexpr uintptr_t fptr_spawn_or_something = 0x008DC540;
    __asm {
			mov eax, [sUnit]
			mov eax, [eax]
			mov esi, obj_to_spawn
			push moveline
			call fptr_spawn_or_something
    }
}

void CustomProjectile::SpawnProjectile() {
    void* projptr = devil4_sdk::mt_allocate_heap(sizeof(CustomProjectileProp),16);
    //void* projptr              = devil4_sdk::mt_allocate_heap(0x18D0, 16);
    CustomProjectileProp *proj = new (projptr) CustomProjectileProp(120.0f, 30.0f,(uActorMain::uCoord*)devil4_sdk::get_local_player(),0);
    sUnit_spawn_call((void*)0x00E552CC, (void*)proj, 1);
}

std::optional<std::string> CustomProjectile::on_initialize() {
    CustomDTI.m_size = sizeof(CustomProjectileProp);
    CustomDTI.m_name = "CustomProjectile";
    CustomDTI.mp_child = 0;
    CustomDTI.mp_link  = 0;
    CustomDTI.mp_next  = 0;
    CustomDTI.mp_parent = (MtDTI*)0xE5B310;
    CustomDTI.m_id      = 7;

    CustomVtable = std::make_unique<CustomProjectileVtable>((void*)uActorVtablePtr, uActorVtableSize);
    
    return Mod::on_initialize();
}

void CustomProjectile::on_gui_frame() {
    if (ImGui::Button("Custom actor")) {
        CustomProjectile::SpawnProjectile();
    }
}