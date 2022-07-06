#pragma once

#include "../mod.hpp"

class InfDT : public Mod {
public:
    InfDT() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static int hotkey;

    std::string get_mod_name() override { return "InfDT"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;
    //void onHotkeySettings(utility::Input& input) override;

private:
    std::unique_ptr<FunctionHook> hook;
    //std::unique_ptr<Hotkey> m_hotkey;
};
