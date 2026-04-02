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

    std::string get_mod_name() override { return "VisualizeHitbox"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hurtboxHook;
};

#pragma pack(push, 1)
struct hurtbox {
    char pad_0x0[0xC];
    float scale; // 0xC
    glm::vec3 offset; // 0x58
}; // Size: 0x5C
#pragma pack(pop)

struct HurtboxSnapshot {
    glm::vec3 pos;
    float radius;
};