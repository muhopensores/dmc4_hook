#pragma once

#include "../mod.hpp"
#include <Xinput.h>

#pragma comment(lib, "XInput.lib")

class PsychoMantis : public Mod {
public:
    PsychoMantis() = default;

    static bool mod_enabled;
    static bool psycho_mantis_slot_2;

    static bool mod_debugEnabled;
    static int mod_debugAmount;
    static int mod_debugLR;

    std::string get_mod_name() override { return "PsychoMantis"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;
    void on_frame(fmilliseconds& dt) override;

    class Controller {
    public:
        Controller() = default;
        ~Controller() = default;
        XINPUT_STATE getState();
        bool isConnected();
        void vibrate(uint16_t left = 0, uint16_t right = 0);
        static int _controllerNum;
    private:
        XINPUT_STATE _controllerState{};
    };

private:
    Controller* _controller;
    Controller* gameController;
};
