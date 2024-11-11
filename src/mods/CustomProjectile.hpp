#pragma once

#include "../mod.hpp"
#include "../sdk/uActor.hpp"

typedef uActorMain::uActor::uActor uActor_t;
typedef uActorMain::uModelMain::uModel uModel_t;


class CustomProjectileVtable {
public:
    std::unique_ptr<uintptr_t[]> my_vtable;
    size_t size;
    CustomProjectileVtable() = default;
    CustomProjectileVtable(void* vtable, size_t size);
};

class CustomProjectileProp{
public:
    uActor_t actor;
    void* CollMgr;
    float keepAliveTime;
    float force;
    CustomProjectileProp() = default;
    CustomProjectileProp(float keepAlive, float force, void* parent, int parentJoint);
    void startup_override();
    void lifecycle_override();
    void onhit_override(void* atk_param, void* dfd_param);
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
    //void on_gui_frame() override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};