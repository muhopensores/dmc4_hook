#include "AfterImage.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/uActor.hpp"

std::unique_ptr<AfterImgCtlVtable> CtlVtable;
std::unique_ptr<AfterImgShlVtable> ShlVtable;
class MtDTI ShlDTI;
class MtDTI CtlDTI;
constexpr uintptr_t rModelDTI           = 0x00EADF48;
constexpr uintptr_t sDevil4Resource_ptr = 0x00E552D0;
constexpr uintptr_t uActorVtablePtr     = 0x00BC4B78; // uActor
constexpr uintptr_t JavelinVtablePtr    = 0x0BD4270;  // uEmShl022Javelin
constexpr size_t uActorVtableSize       = 79;

typedef uActorMain::uActor::uActor uActor_t;
typedef uActorMain::uModelMain::uModel uModel_t;
typedef uActorMain::uDevil4Model uDevil4Model_t;
typedef uActorMain::uCollisionMgr uCollisionMgr;

struct AfterImgCtl {
public:
    uActor_t actor;
    uActor_t* parent;
    bool canSpawn;
    float cooldown;
    float cooldown_default;
    AfterImgCtl() = default;
    AfterImgCtl(uActor_t* parent);
    void destructor(uint32_t flag);
    void die();
    void startup_override();
    void lifecycle_override();
    // void render(void* smth);
    MtDTI* getDTI();
};

struct AfterImgShl {
public:
    uActor_t actor;
    uDevil4Model_t hair;
    uDevil4Model_t head;
    uDevil4Model_t jacket;
    uActor_t* parent;
    bool fade;
    float keepAliveTime;
    float fadeTime;
    AfterImgShl() = default;
    AfterImgShl(uActor_t* parent);
    void destructor(uint32_t flag);
    void die();
    void startup_override();
    void lifecycle_override();
    void render(void* trans);
    MtDTI* getDTI();
};

struct uModel_Joint {
    void* vtable;
    byte mAttr;
    byte mParentIndex;
    byte mType;
    byte mNo;
    int mSymmetryIndex;
    void* mpConstraint;
    struct uActorMain::MtFloat3 mOffset;
    float mLength;
    struct uActorMain::MtVector4 mQuat;
    struct uActorMain::MtVector3 mScale;
    struct uActorMain::MtVector3 mTrans;
    struct MtMatrix mWmat;
};
static_assert(sizeof(uModel_Joint)==0x90);

AfterImgCtlVtable::AfterImgCtlVtable(void* vtable, size_t size) {
    this->size      = size;
    this->my_vtable = std::vector<uintptr_t>((uintptr_t*)vtable, (uintptr_t*)vtable + size);
    // memcpy(this->my_vtable.data() + 0x24, (void*)(JavelinVtablePtr + 0x24), 4);
    this->my_vtable[0]        = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::destructor);
    this->my_vtable[0x10 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::getDTI);
    //this->my_vtable[0x14 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::startup_override);
    this->my_vtable[0x30 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::die);
    this->my_vtable[0x78 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::lifecycle_override);
}

void AfterImgCtl::destructor(uint32_t flag) {
    uactor_sdk::ushell_des(this);
    if (flag) {
        devil4_sdk::unit_deallocate((MtObject*)this);
    }
}

void AfterImgCtl::die() {
    uactor_sdk::despawn(this);
}


MtDTI* AfterImgCtl::getDTI() {
    return &CtlDTI;
}

void AfterImgCtl::startup_override() {
    return;
}

void AfterImgCtl::lifecycle_override() {
    if ((this->canSpawn) && (*(float*)((uintptr_t)this->parent + 0x1E1C) > 5.0f)) {
        AfterImage::SpawnAfterImg();
        this->canSpawn = false;
    }
    if (this->cooldown <= 0.0f) {
        this->cooldown = this->cooldown_default;
        this->canSpawn = true;
    } else if (!this->canSpawn)
        this->cooldown -= this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.m_delta_time;
}

AfterImgCtl::AfterImgCtl(uActor_t* parent) {
    uactor_sdk::uActorCons(this);
    this->parent = parent;
    this->cooldown = 3.0f;
    this->cooldown_default = 3.0f;
    this->canSpawn = false;
    this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.vtable_ptr = (uintptr_t*)CtlVtable->my_vtable.data();
    this->actor.mActorType                                            = 5;
    this->actor.uActorBase.mWorkRate.mType                            = 0x9;
}

AfterImgShlVtable::AfterImgShlVtable(void* vtable, size_t size) {
    this->size      = size;
    this->my_vtable = std::vector<uintptr_t>((uintptr_t*)vtable, (uintptr_t*)vtable + size);
    // memcpy(this->my_vtable.data() + 0x24, (void*)(JavelinVtablePtr + 0x24), 4);
    this->my_vtable[0]        = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgShl::destructor);
    this->my_vtable[0x10 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgShl::getDTI);
    this->my_vtable[0x14 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgShl::startup_override);
    this->my_vtable[0x24 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgShl::render);
    this->my_vtable[0x30 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgShl::die);
    this->my_vtable[0x78 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgShl::lifecycle_override);
}

void AfterImgShl::destructor(uint32_t flag) {
    uactor_sdk::uDevil4ModelDest(&this->jacket);
    uactor_sdk::uDevil4ModelDest(&this->head);
    uactor_sdk::uDevil4ModelDest(&this->hair);
    uactor_sdk::ushell_des(this);
    
    if (flag) {
        devil4_sdk::unit_deallocate((MtObject*)this);
    }
}

void AfterImgShl::die() {
    uactor_sdk::despawn(this);
    uactor_sdk::despawn(&this->jacket);
    uactor_sdk::despawn(&this->hair);
    uactor_sdk::despawn(&this->head);
}

MtDTI* AfterImgShl::getDTI() {
    return &ShlDTI;
}

void AfterImgShl::render(void* trans) {
    void* curr_context = *(void**)((uintptr_t)trans+0x2A90);
    uint8_t curr_flag    = *(short*)((uintptr_t)curr_context + 0x1FC);
    if (curr_flag & this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.mTransMode)
        uactor_sdk::render_call(this, trans);
    if (curr_flag & this->hair.uModelBase.uCoordBase.cUnitBase.mTransMode)
        uactor_sdk::render_call(&this->hair, trans);
    if (curr_flag & this->head.uModelBase.uCoordBase.cUnitBase.mTransMode)
        uactor_sdk::render_call(&this->head, trans);
    if (curr_flag & this->jacket.uModelBase.uCoordBase.cUnitBase.mTransMode)
        uactor_sdk::render_call(&this->jacket, trans);
}

void AfterImgShl::startup_override() {
    uDevil4Model_t* body_model = (uDevil4Model_t*)this->parent;
    uDevil4Model_t* face_model = (uDevil4Model_t*)((uintptr_t)this->parent+0x3270);
    uDevil4Model_t* hair_model   = (uDevil4Model_t*)((uintptr_t)this->parent + 0x4340);
    uDevil4Model_t* jacket_model = (uDevil4Model_t*)((uintptr_t)this->parent + 0x52C0);
    uactor_sdk::get_model(this, (void*)body_model->uModelBase.mpRModel);
    uactor_sdk::get_model(&this->head, (void*)face_model->uModelBase.mpRModel);
    uactor_sdk::get_model(&this->hair, (void*)hair_model->uModelBase.mpRModel);
    uactor_sdk::get_model(&this->jacket, (void*)jacket_model->uModelBase.mpRModel);

    this->actor.uActorBase.mTransparency  = 0.3f;
    this->jacket.mTransparency = 0.3f;
    this->head.mTransparency = 0.1f;
    this->hair.mTransparency = 0.1f;

    int body_jnt_num = body_model->uModelBase.mJointNum;
    uModel_Joint* actor_jnt_ptr = (uModel_Joint*)this->actor.uActorBase.uModelBase.mpJoint;
    uModel_Joint* parent_jnt_ptr = (uModel_Joint*)body_model->uModelBase.mpJoint;
    for (int i = 0; i < body_jnt_num; i++) {
        actor_jnt_ptr[i].mWmat = parent_jnt_ptr[i].mWmat;
    }

    int jacket_jnt_num = jacket_model->uModelBase.mJointNum;
    uModel_Joint* actor_jacket_jnt_ptr  = (uModel_Joint*)this->jacket.uModelBase.mpJoint;
    uModel_Joint* parent_jacket_jnt_ptr = (uModel_Joint*)jacket_model->uModelBase.mpJoint;
    for (int i = 0; i < jacket_jnt_num; i++) {
        actor_jacket_jnt_ptr[i].mWmat = parent_jacket_jnt_ptr[i].mWmat;
        actor_jacket_jnt_ptr[i].mpConstraint = parent_jacket_jnt_ptr[i].mpConstraint;
    }

    uModel_Joint* actor_head_jnt_ptr  = (uModel_Joint*)this->head.uModelBase.mpJoint;
    uModel_Joint* parent_head_jnt_ptr = (uModel_Joint*)face_model->uModelBase.mpJoint;
    for (int i = 0; i < face_model->uModelBase.mJointNum; i++) {
        actor_head_jnt_ptr[i].mWmat        = parent_head_jnt_ptr[i].mWmat;
        actor_head_jnt_ptr[i].mpConstraint = parent_head_jnt_ptr[i].mpConstraint;
    }

    uModel_Joint* actor_hair_jnt_ptr  = (uModel_Joint*)this->hair.uModelBase.mpJoint;
    uModel_Joint* parent_hair_jnt_ptr = (uModel_Joint*)hair_model->uModelBase.mpJoint;
    actor_hair_jnt_ptr[0].mWmat       = parent_hair_jnt_ptr[0].mWmat;
    for (int i = 0; i < hair_model->uModelBase.mJointNum; i++) {
        actor_hair_jnt_ptr[i].mWmat        = parent_hair_jnt_ptr[i].mWmat;
        actor_hair_jnt_ptr[i].mpConstraint = parent_hair_jnt_ptr[i].mpConstraint;
    }
    //memcpy((void*)this->actor.uActorBase.uModelBase.mpJoint, (void*)body_model->uModelBase.mpJoint, 0x90 * body_jnt_num);
    //int jacket_jnt_num = jacket_model->uModelBase.mJointNum;
    //memcpy((void*)this->jacket.uModelBase.mpJoint, (void*)jacket_model->uModelBase.mpJoint, 0x90 * jacket_jnt_num);
    return;
}

void AfterImgShl::lifecycle_override() {
    //uactor_sdk::updateLmat(this);
    if (fade) {
        if (this->fadeTime < 0.0f)
            this->die();
        else {
            this->fadeTime -= this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.m_delta_time;
            this->actor.uActorBase.mTransparency *= fadeTime / 10.0f;
            this->jacket.mTransparency *= fadeTime / 10.0f;
            this->head.mTransparency *= fadeTime / 10.0f;
            this->hair.mTransparency *= fadeTime / 10.0f;
        }
    }
    uactor_sdk::updateWmat(&this->jacket);
    this->keepAliveTime -= this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.m_delta_time;
    if (this->keepAliveTime < 0.0f)
        this->fade = true;
}

AfterImgShl::AfterImgShl(uActor_t* parent) {
    this->parent = parent;
    uactor_sdk::uActorCons(this);

    uactor_sdk::uDevil4ModelCons(&this->head);
    this->head.uModelBase.uCoordBase.mParent = (uintptr_t)this;
    this->head.uModelBase.uCoordBase.ParentJoint = -1;

    uactor_sdk::uDevil4ModelCons(&this->hair);
    this->hair.uModelBase.uCoordBase.mParent = (uintptr_t)this;
    this->hair.uModelBase.uCoordBase.ParentJoint = -1;

    uactor_sdk::uDevil4ModelCons(&this->jacket);
    this->jacket.uModelBase.uCoordBase.mParent = (uintptr_t)this;
    this->jacket.uModelBase.uCoordBase.ParentJoint = 2;

    this->fade = false;
    this->fadeTime = 10.0f;
    this->keepAliveTime = 30.0f;
    this->actor.uActorBase.uModelBase.uCoordBase.cUnitBase.vtable_ptr = (uintptr_t*)ShlVtable->my_vtable.data();
    this->actor.mActorType                                            = 5;
    this->actor.uActorBase.mWorkRate.mType                            = 0x9;
    this->actor.uActorBase.uModelBase.uCoordBase.mPos.x               = parent->uActorBase.uModelBase.uCoordBase.mPos.x;
    this->actor.uActorBase.uModelBase.uCoordBase.mPos.y               = parent->uActorBase.uModelBase.uCoordBase.mPos.y;
    this->actor.uActorBase.uModelBase.uCoordBase.mPos.z               = parent->uActorBase.uModelBase.uCoordBase.mPos.z;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.x              = parent->uActorBase.uModelBase.uCoordBase.mQuat.x;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.y              = parent->uActorBase.uModelBase.uCoordBase.mQuat.y;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.z              = parent->uActorBase.uModelBase.uCoordBase.mQuat.z;
    this->actor.uActorBase.uModelBase.uCoordBase.mQuat.w              = parent->uActorBase.uModelBase.uCoordBase.mQuat.w;
}


void AfterImage::SpawnAfterImg() {
    void* projptr = devil4_sdk::mt_allocate_heap(sizeof(AfterImgShl), 16);
    // void* projptr              = devil4_sdk::mt_allocate_heap(0x18D0, 16);
    AfterImgShl* proj = new (projptr) AfterImgShl((uActor_t*)devil4_sdk::get_local_player());
    devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)proj, 1);
}

void AfterImage::SpawnAfterImgCtl() {
    void* projptr = devil4_sdk::mt_allocate_heap(sizeof(AfterImgCtl), 16);
    // void* projptr              = devil4_sdk::mt_allocate_heap(0x18D0, 16);
    AfterImgCtl* proj = new (projptr) AfterImgCtl((uActor_t*)devil4_sdk::get_local_player());
    devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)proj, 1);
}

std::optional<std::string> AfterImage::on_initialize() {
    ShlDTI.m_size    = sizeof(AfterImgShl);
    ShlDTI.m_name       = "AfterImageShl";
    ShlDTI.mp_child     = 0;
    ShlDTI.mp_link      = 0;
    ShlDTI.mp_next      = 0;
    ShlDTI.mp_parent    = (MtDTI*)0xE5B310;
    ShlDTI.m_id         = 7;

    CtlDTI.m_size    = sizeof(AfterImgCtl);
    CtlDTI.m_name    = "AfterImageShlCtl";
    CtlDTI.mp_child  = 0;
    CtlDTI.mp_link   = 0;
    CtlDTI.mp_next   = 0;
    CtlDTI.mp_parent = (MtDTI*)0xE5B310;
    CtlDTI.m_id      = 8;

    CtlVtable = std::make_unique<AfterImgCtlVtable>((void*)uActorVtablePtr, uActorVtableSize);
    ShlVtable = std::make_unique<AfterImgShlVtable>((void*)uActorVtablePtr, uActorVtableSize);
    return Mod::on_initialize();
}

void AfterImage::on_gui_frame(int display) {
    if (ImGui::Button("After image")) {
        // if (fileExists)
        AfterImage::SpawnAfterImg();
    }
    if (ImGui::Button("After image controller")) {
        // if (fileExists)
        AfterImage::SpawnAfterImgCtl();
    }
}
