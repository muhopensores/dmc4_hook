#pragma once

#include "../mod.hpp"
#include "../sdk/uActor.hpp"
#ifndef CUSTOMPROJECTILE_H
#define CUSTOMPROJECTILE_H

typedef uActorMain::uActor::uActor uActor_t;
typedef uActorMain::uModelMain::uModel uModel_t;
typedef uActorMain::uCollisionMgr uCollisionMgr;

class CustomProjectileVtable {
public:
    std::vector<uintptr_t> my_vtable;
    size_t size;
    CustomProjectileVtable() = default;
    CustomProjectileVtable(void* vtable, size_t size);
};

struct CustomProjectileProp{
public:
    uActor_t actor;
    uCollisionMgr CollMgr;
    float keepAliveTime;
    float HitTimer;
    float KeepAliveTimer;
    float force;
    CustomProjectileProp() = default;
    CustomProjectileProp(float keepAlive, float force, uActorMain::uCoord* parent, int parentJoint);
    CustomProjectileProp(float keepAlive, float force, Vector3f* Pos, Vector4f* Quat);
    void destructor(uint32_t flag);
    void die();
    void startup_override();
    void lifecycle_override();
    void onhit_override(void* atk_param, void* dfd_param);
    void render(void* smth);
    MtDTI* getDTI();
};

class CustomProjectile: public Mod {
public:
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static bool mod_enabled;

    CustomProjectile() = default;

    static void __stdcall SpawnProjectile();
    std::string get_mod_name() override { return "CustomProjectile"; };
    std::optional<std::string> on_initialize() override;
    //void on_config_load(const utility::Config& cfg) override;
    //void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};
#endif