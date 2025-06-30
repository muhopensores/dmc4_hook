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

char* BODY_PATH = "model\\game\\pl006\\pl006_afimg";
char* HEAD_PATH   = "model\\game\\pl006\\pl006_01_afimg";
char* HAIR_PATH   = "model\\game\\pl006\\pl006_02_afimg";
char* JACKET_PATH   = "model\\game\\pl006\\pl006_03_afimg";

typedef uActor uActor_t;
typedef uModel uModel_t;
typedef uDevil4Model uDevil4Model_t;

struct AfterImgCtl {
public:
    uActor_t actor;
    uActor_t* parent;
    void* BodyModel;
    void* HeadModel;
    void* HairModel;
    void* JacketModel;
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
    AfterImgCtl* spawnParent;
    bool fade;
    float keepAliveTime;
    float fadeTime;

    AfterImgShl() = default;
    AfterImgShl(uActor_t* parent, AfterImgCtl* spawnParent);
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
    byte mpParentIndex;
    byte mType;
    byte mNo;
    int mSymmetryIndex;
    void* mpConstraint;
    struct MtFloat3 mOffset;
    float mLength;
    MtVector4 mQuat;
    MtVector3 mScale;
    MtVector3 mTrans;
    MtMatrix mWmat;
};
static_assert(sizeof(uModel_Joint)==0x90);

union BLENDSTATE {
    uint32_t blend_state;
    struct {
        uint32_t destblend_alpha : 8;
        uint32_t blendop_alpha : 3;
        uint32_t srcblend_alpha : 5;
        uint32_t destblend : 8;
        uint32_t blendop : 3;
        uint32_t srcblend : 5;
    };
};

union DRAWSTATE {
    uint32_t draw_state;
    struct {
        uint32_t reserved : 3;
        uint32_t cullflip : 1;
        uint32_t pointsprite : 1;
        uint32_t scissor : 1;
        uint32_t colorwrite : 4;
        uint32_t cullmode : 3;
        uint32_t alpharef : 8;
        uint32_t alphafunc : 4;
        uint32_t zfunc : 4;
        uint32_t alphatest : 1;
        uint32_t zwrite : 1;
        uint32_t zenable : 1;
    };
};

struct alignas(16) cMaterial {
    uintptr_t vtable;
    //uint32_t uknFlag1;
    uint32_t type: 8;
    uint32_t alphaRef: 8; //16
    uint32_t blend: 7; // 23
    uint32_t ukn: 4; //27
    uint32_t zwrite: 1; //28
    uint32_t zthrough: 1; //29
    uint32_t solidpriority: 2;
    uint32_t uknFlag2;
    uint32_t TechniqueNum;
    uint32_t PipelineNum;
    BLENDSTATE blend_state;
    DRAWSTATE draw_state;
    uintptr_t BaseMap;
    float Transparency;
    int padding[3];
    Vector4f BaseMapFactor;
    Vector3f DiffuseFactor;
    float SpecularFactor;
    char VectorGroups[4];
};
//static_assert(offsetof(cMaterial, BaseMapFactor) == 0x24);
static_assert(sizeof(cMaterial) == 0x60);

AfterImgCtlVtable::AfterImgCtlVtable(void* vtable, size_t size) {
    this->size      = size;
    this->my_vtable = std::vector<uintptr_t>((uintptr_t*)vtable, (uintptr_t*)vtable + size);
    // memcpy(this->my_vtable.data() + 0x24, (void*)(JavelinVtablePtr + 0x24), 4);
    this->my_vtable[0]        = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::destructor);
    this->my_vtable[0x10 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::getDTI);
    this->my_vtable[0x14 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::startup_override);
    this->my_vtable[0x30 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::die);
    this->my_vtable[0x78 / 4] = (uintptr_t)devil4_sdk::MemberFuncToPtr(&AfterImgCtl::lifecycle_override);
}

void AfterImgCtl::destructor(uint32_t flag) {
    uactor_sdk::ushell_des(this);
    //devil4_sdk::release_resource((CResource*)this->BodyModel);
    //devil4_sdk::release_resource((CResource*)this->HeadModel);
    //devil4_sdk::release_resource((CResource*)this->HairModel);
    //devil4_sdk::release_resource((CResource*)this->JacketModel);
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
    void* body_model = devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, BODY_PATH, 1);
    this->BodyModel = body_model;
    void* head_model = devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, HEAD_PATH, 1);
    this->HeadModel  = head_model;
    void* hair_model = devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, HAIR_PATH, 1);
    this->HairModel    = hair_model;
    void* jacket_model = devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, JACKET_PATH, 1);
    this->JacketModel = jacket_model;
    //devil4_sdk::bring_assert(body_model);
    //devil4_sdk::bring_assert(head_model);
    //devil4_sdk::bring_assert(hair_model);
    //devil4_sdk::bring_assert(jacket_model);
    return;
}

void AfterImgCtl::lifecycle_override() {
    if ((this->canSpawn) && (*(float*)((uintptr_t)this->parent + 0x1E1C) > 5.0f)) {
        AfterImage::SpawnAfterImg(this);
        this->canSpawn = false;
    }
    if (this->cooldown <= 0.0f) {
        this->cooldown = this->cooldown_default;
        this->canSpawn = true;
    } else if (!this->canSpawn)
        this->cooldown -= this->actor.m_delta_time;
}

AfterImgCtl::AfterImgCtl(uActor_t* parent) {
    uactor_sdk::uActorCons(this);
    this->parent = parent;
    this->cooldown = 5.0f;
    this->cooldown_default = 5.0f;
    this->canSpawn = false;
    this->actor.vtable_ptr = (uintptr_t*)CtlVtable->my_vtable.data();
    this->actor.mActorType                                            = 5;
    this->actor.mWorkRate.mType                            = 0x9;
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
    uint8_t curr_flag   = *(short*)((uintptr_t)curr_context + 0x1FC);
    void* body_ptr     = (void*)this->actor.mpRModel;
    void* hair_ptr     = (void*)this->hair.mpRModel;
    void* head_ptr     = (void*)this->head.mpRModel;
    void* jacket_ptr   = (void*)this->jacket.mpRModel;
    if (curr_flag && this->actor.flags.bits.mTransMode && body_ptr != nullptr)
        uactor_sdk::render_call(this, trans);
    if (curr_flag && this->hair.flags.bits.mTransMode && hair_ptr != nullptr)
        uactor_sdk::render_call(&this->hair, trans);
    if (curr_flag && this->head.flags.bits.mTransMode && head_ptr != nullptr)
        uactor_sdk::render_call(&this->head, trans);
    if (curr_flag && this->jacket.flags.bits.mTransMode && jacket_ptr != nullptr)
        uactor_sdk::render_call(&this->jacket, trans);
}

void AfterImgShl::startup_override() {
    uDevil4Model_t* body_model = (uDevil4Model_t*)this->parent;
    uDevil4Model_t* face_model = (uDevil4Model_t*)((uintptr_t)this->parent+0x3270);
    uDevil4Model_t* hair_model   = (uDevil4Model_t*)((uintptr_t)this->parent + 0x4340);
    uDevil4Model_t* jacket_model = (uDevil4Model_t*)((uintptr_t)this->parent + 0x52C0);
    if (this->spawnParent != nullptr) {
        if (spawnParent->BodyModel != nullptr)
            uactor_sdk::get_model(this, (void*)spawnParent->BodyModel);
        else
            uactor_sdk::get_model(this, (void*)body_model->mpRModel);

        if (spawnParent->HeadModel != nullptr)
            uactor_sdk::get_model(&this->head, (void*)spawnParent->HeadModel);
        else
            uactor_sdk::get_model(&this->head, (void*)face_model->mpRModel);

        if (spawnParent->HairModel != nullptr)
            uactor_sdk::get_model(&this->hair, (void*)spawnParent->HairModel);
        else
            uactor_sdk::get_model(&this->hair, (void*)hair_model->mpRModel);

        if (spawnParent->JacketModel != nullptr)
            uactor_sdk::get_model(&this->jacket, (void*)spawnParent->JacketModel);
        else
            uactor_sdk::get_model(&this->jacket, (void*)jacket_model->mpRModel);
        //devil4_sdk::bring_assert(spawnParent->JacketModel);
    } else {
        uactor_sdk::get_model(this, (void*)body_model->mpRModel);
        uactor_sdk::get_model(&this->head, (void*)face_model->mpRModel);
        uactor_sdk::get_model(&this->hair, (void*)hair_model->mpRModel);
        uactor_sdk::get_model(&this->jacket, (void*)jacket_model->mpRModel);
    }
    
    this->actor.mCameraTransparencyEnable = false;
    this->actor.Render.mVFCullLevel = 0;
    this->actor.Render.mRenderMode = 0;
    this->head.Render.mVFCullLevel  = 0;
    this->head.Render.mRenderMode   = 0;
    this->hair.Render.mVFCullLevel              = 0;
    this->hair.Render.mRenderMode               = 0;
    this->actor.Render.mZPrepassDist  = 2000;
    this->actor.Render.mPriorityBias = -100;
    //this->actor.mTransparency  = 0.3f;
    //this->jacket.mTransparency = 0.3f;
    //this->head.mTransparency = 0.1f;
    //this->hair.mTransparency = 0.1f;
    
    int body_jnt_num = body_model->mJointNum;
    uModel_Joint* actor_jnt_ptr = (uModel_Joint*)this->actor.mpJoint;
    uModel_Joint* parent_jnt_ptr = (uModel_Joint*)body_model->mpJoint;
    for (int i = 0; i < body_jnt_num; i++) {
        actor_jnt_ptr[i].mWmat = parent_jnt_ptr[i].mWmat;
    }

    int jacket_jnt_num = jacket_model->mJointNum;
    uModel_Joint* actor_jacket_jnt_ptr  = (uModel_Joint*)this->jacket.mpJoint;
    uModel_Joint* parent_jacket_jnt_ptr = (uModel_Joint*)jacket_model->mpJoint;
    for (int i = 0; i < jacket_jnt_num; i++) {
        actor_jacket_jnt_ptr[i].mWmat = parent_jacket_jnt_ptr[i].mWmat;
        //actor_jacket_jnt_ptr[i].mpConstraint = parent_jacket_jnt_ptr[i].mpConstraint;
    }

    uModel_Joint* actor_head_jnt_ptr  = (uModel_Joint*)this->head.mpJoint;
    uModel_Joint* parent_head_jnt_ptr = (uModel_Joint*)face_model->mpJoint;
    for (int i = 0; i < face_model->mJointNum; i++) {
        actor_head_jnt_ptr[i].mWmat        = parent_head_jnt_ptr[i].mWmat;
        actor_head_jnt_ptr[i].mpConstraint = parent_head_jnt_ptr[i].mpConstraint;
    }

    uModel_Joint* actor_hair_jnt_ptr  = (uModel_Joint*)this->hair.mpJoint;
    uModel_Joint* parent_hair_jnt_ptr = (uModel_Joint*)hair_model->mpJoint;
    actor_hair_jnt_ptr[0].mWmat       = parent_hair_jnt_ptr[0].mWmat;
    for (int i = 0; i < hair_model->mJointNum; i++) {
        actor_hair_jnt_ptr[i].mWmat        = parent_hair_jnt_ptr[i].mWmat;
        actor_hair_jnt_ptr[i].mpConstraint = parent_hair_jnt_ptr[i].mpConstraint;
    }


    cMaterial** mat_arr = (cMaterial**)this->actor.mpMaterial;
    uint32_t mat_num   = this->actor.MaterialNum;
    for (int i = 0; i < mat_num; i++) {
        cMaterial* mat = mat_arr[i];
        mat->BaseMapFactor.x        = 0.1f;
        mat->BaseMapFactor.y       = 0.1f;
        mat->BaseMapFactor.z        = 0.1f;
        mat->Transparency           = 0.2f;
        mat->SpecularFactor         = 5.0f;
        mat->zwrite = 0;
        mat->draw_state.zwrite = 0;
        mat->zthrough                 = 1;
    }

    mat_arr = (cMaterial**)this->jacket.mpMaterial;
    mat_num = this->jacket.MaterialNum;
    for (int i = 0; i < mat_num; i++) {
        cMaterial* mat       = mat_arr[i];
        mat->BaseMapFactor.x = 0.1f;
        mat->BaseMapFactor.y = 0.1f;
        mat->BaseMapFactor.z = 0.1f;
        mat->Transparency    = 0.2f;
        mat->SpecularFactor  = 5.0f;
        mat->zwrite          = 0;
        mat->draw_state.zwrite = 0;
        mat->zthrough        = 1;
    }
    mat_arr = (cMaterial**)this->head.mpMaterial;
    mat_num = this->head.MaterialNum;
    for (int i = 0; i < mat_num; i++) {
        cMaterial* mat       = mat_arr[i];
        mat->BaseMapFactor.x = 0.1f;
        mat->BaseMapFactor.y = 0.1f;
        mat->BaseMapFactor.z = 0.1f;
        mat->Transparency    = 0.2f;
        mat->SpecularFactor  = 5.0f;
        mat->zwrite          = 0;
        mat->draw_state.zwrite = 0;
        mat->zthrough        = 1;
    }
    mat_arr = (cMaterial**)this->hair.mpMaterial;
    mat_num = this->hair.MaterialNum;
    for (int i = 0; i < mat_num; i++) {
        cMaterial* mat       = mat_arr[i];
        mat->BaseMapFactor.x = 0.1f;
        mat->BaseMapFactor.y = 0.1f;
        mat->BaseMapFactor.z = 0.1f;
        mat->Transparency    = 0.2f;
        mat->SpecularFactor  = 5.0f;
        mat->zwrite          = 0;
        mat->draw_state.zwrite = 0;
        mat->zthrough        = 1;
    }
    //memcpy((void*)this->actor.mpJoint, (void*)body_model->mpJoint, 0x90 * body_jnt_num);
    //int jacket_jnt_num = jacket_model->mJointNum;
    //memcpy((void*)this->jacket.mpJoint, (void*)jacket_model->mpJoint, 0x90 * jacket_jnt_num);
    return;
}

void AfterImgShl::lifecycle_override() {
    //uactor_sdk::updateLmat(this);
    void* body_ptr = (void*)this->actor.mpRModel;
    void* hair_ptr = (void*)this->hair.mpRModel;
    void* head_ptr   = (void*)this->head.mpRModel;
    void* jacket_ptr = (void*)this->jacket.mpRModel;
    if (body_ptr == nullptr || hair_ptr == nullptr || head_ptr == nullptr || jacket_ptr == nullptr)
        this->die();
    if (fade) {
        if (this->fadeTime < 0.0f)
            this->die();
        else {
            this->fadeTime -= this->actor.m_delta_time;
            this->actor.mTransparency *= fadeTime / 10.0f;
            this->jacket.mTransparency *= fadeTime / 10.0f;
            this->head.mTransparency *= fadeTime / 10.0f;
            this->hair.mTransparency *= fadeTime / 10.0f;
        }
    }
    uactor_sdk::updateWmat(&this->jacket);
    this->keepAliveTime -= this->actor.m_delta_time;
    if (this->keepAliveTime < 0.0f)
        this->fade = true;
}

AfterImgShl::AfterImgShl(uActor_t* parent, AfterImgCtl* spawnParent) {
    this->parent = parent;
    this->spawnParent = spawnParent;
    uactor_sdk::uActorCons(this);

    uactor_sdk::uDevil4ModelCons(&this->head);
    this->head.mpParent = (uCoord*)this;
    this->head.ParentJoint = 4;

    uactor_sdk::uDevil4ModelCons(&this->hair);
    this->hair.mpParent    = (uCoord*)this;
    this->hair.ParentJoint = 4;

    uactor_sdk::uDevil4ModelCons(&this->jacket);
    this->jacket.mpParent    = (uCoord*)this;
    this->jacket.ParentJoint = 2;

    this->fade = false;
    this->fadeTime = 10.0f;
    this->keepAliveTime = 20.0f;
    this->actor.vtable_ptr = (uintptr_t*)ShlVtable->my_vtable.data();
    this->actor.mActorType                                            = 5;
    this->actor.mWorkRate.mType                            = 0x9;
    this->actor.mPos.x               = parent->mPos.x;
    this->actor.mPos.y               = parent->mPos.y;
    this->actor.mPos.z               = parent->mPos.z;
    this->actor.mQuat.x              = parent->mQuat.x;
    this->actor.mQuat.y              = parent->mQuat.y;
    this->actor.mQuat.z              = parent->mQuat.z;
    this->actor.mQuat.w              = parent->mQuat.w;
}


void AfterImage::SpawnAfterImg(void* spawnParent = nullptr) {
    void* projptr = devil4_sdk::mt_allocate_heap(sizeof(AfterImgShl), 16);
    // void* projptr              = devil4_sdk::mt_allocate_heap(0x18D0, 16);
    AfterImgShl* proj = new (projptr) AfterImgShl((uActor_t*)devil4_sdk::get_local_player(), (AfterImgCtl*)spawnParent);
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
