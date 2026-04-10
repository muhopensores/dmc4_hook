#pragma once

#include "../mod.hpp"

class VisualizeHitbox : public Mod {
public:
    VisualizeHitbox() = default;

    static bool mod_enabled;
    static bool mod_enabled2;
    static uintptr_t jmp_ret_hurtboxes;
    static bool mod_enabled3;
    static bool mod_enabled4;
    static uintptr_t jmp_ret_hitboxes2;
    std::string get_mod_name() override { return "VisualizeHitbox"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
 
private:
    // these do not work for me so are placed at the top of the cpp
    std::unique_ptr<FunctionHook> hurtboxHook;
    std::unique_ptr<FunctionHook> hitbox2hook;
};

struct hurtboxData {
    char pad_0x0[0xC];
    float radius; // 0xC-0x10
    glm::vec3 offset; // 0x10
};
struct hurtboxPos {
    char pad_0x0[0xF0];
    glm::vec3 pos; // 0xF0
};
struct HurtboxSnapshot {
    glm::vec3 pos;
    float radius;
};

struct HitboxSnapshot {
    glm::vec3 pos;
    float radius;
    bool isCapsule  = false;
    float length    = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;

    // sphere
    HitboxSnapshot(glm::vec3 p, float r)
        : pos(p), radius(r), isCapsule(false), length(0.0f), rotationX(0.0f), rotationY(0.0f) {}

    // capsule
    HitboxSnapshot(glm::vec3 p, float r, float len, float rotX, float rotY)
        : pos(p), radius(r), isCapsule(true), length(len), rotationX(rotX), rotationY(rotY) {}
};
