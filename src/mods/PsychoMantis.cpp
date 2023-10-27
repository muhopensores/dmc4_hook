#include "PsychoMantis.hpp"

// https://gist.github.com/stroum
bool PsychoMantis::mod_enabled{false};
bool PsychoMantis::psycho_mantis_slot_2{false};

PsychoMantis::Controller::Controller() {
    if (PsychoMantis::psycho_mantis_slot_2)
        _controllerNum = 1;
    else
        _controllerNum = 0;
}

PsychoMantis::Controller::~Controller() {
}

XINPUT_STATE PsychoMantis::Controller::getState() {
    ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

    XInputGetState(_controllerNum, &_controllerState);

    return _controllerState;
}

bool PsychoMantis::Controller::isConnected() {
    ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

    DWORD Result = XInputGetState(_controllerNum, &_controllerState);

    return Result == ERROR_SUCCESS;
}

void PsychoMantis::Controller::vibrate(uint16_t left, uint16_t right) {
    XINPUT_VIBRATION v;

    ZeroMemory(&v, sizeof(XINPUT_VIBRATION));

    v.wLeftMotorSpeed  = left;
    v.wRightMotorSpeed = right;

    XInputSetState(_controllerNum, &v);
}

std::optional<std::string> PsychoMantis::on_initialize() {
    return Mod::on_initialize();
}

void PsychoMantis::on_gui_frame() {
    ImGui::Checkbox("Finally... DMC4 2", &mod_enabled);
    ImGui::SameLine();
    help_marker("Put your controller on the floor. Put it down as flat as you can..."
        "that's good. Now I will move your controller by the power of my style alone!!");
    ImGui::SameLine(sameLineWidth);
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::Checkbox("Use Slot 2", &psycho_mantis_slot_2);
    ImGui::PopItemWidth();
}
void PsychoMantis::on_frame(fmilliseconds& dt) {
    if (mod_enabled && !devil4_sdk::is_paused() && devil4_sdk::get_work_rate()->global_speed != 0.0f) {
        Controller* c = new Controller();
        uint16_t lpower = 65535 / 14; // apparently 65535 is the limit but there isn't much difference if any from half the amount
        uint16_t rpower = 65535 / 14; // so we're divibing by double so its smoother
        if (c->isConnected()) {
            c->vibrate(lpower * devil4_sdk::get_current_style_rank(), rpower * devil4_sdk::get_current_style_rank());
        }
        /* if (c->getState().Gamepad.wButtons & XINPUT_GAMEPAD_A) {
            c->vibrate(65535, 65535);
        }
        if (c->getState().Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
            c->vibrate(65535 / 2, 65535 / 2);
        }*/
    }
}

void PsychoMantis::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("psycho_mantis_toggle").value_or(false);
    psycho_mantis_slot_2 = cfg.get<bool>("psycho_mantis_slot_2").value_or(false);
}

void PsychoMantis::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("psycho_mantis_toggle", mod_enabled);
    cfg.set<bool>("psycho_mantis_slot_2", psycho_mantis_slot_2);
}
