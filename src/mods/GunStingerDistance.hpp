#pragma once

#include "../mod.hpp"

class GunStingerDistance : public Mod {
public:
    GunStingerDistance() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;

    std::string get_mod_name() override { return "GunStingerDistance"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
