#include "PsychoMantis.hpp"

// https://gist.github.com/stroum
bool PsychoMantis::mod_enabled{false};
bool PsychoMantis::psycho_mantis_slot_2{false};
bool PsychoMantis::mod_debugEnabled{false};
int PsychoMantis::mod_debugAmount{0};
int PsychoMantis::mod_debugLR{0};
int PsychoMantis::Controller::_controllerNum = 0;

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
    PsychoMantis::_controller = new Controller();
    return Mod::on_initialize();
}

void PsychoMantis::on_gui_frame() {
    ImGui::Checkbox(_("Vibe"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Put your controller on the floor. Put it down as flat as you can..."
        "that's good. Now I will move your controller by the power of my style alone!!"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Vibe Slot 2"), &psycho_mantis_slot_2)) {
        if (psycho_mantis_slot_2){
            mod_enabled = true;
            PsychoMantis::Controller::_controllerNum = 1;
        } else
            PsychoMantis::Controller::_controllerNum = 0;
    }
#ifndef NDEBUG
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("...debug"), &mod_debugEnabled);
        ImGui::SameLine();
        help_marker(_("Tick the Vibe checkbox too"));
        ImGui::SameLine();
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::SliderInt(_("##mod_enabled2AmountSliderInt"), &mod_debugAmount, 0, 65535);
        ImGui::PopItemWidth();

        ImGui::SameLine(sameLineWidth);
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::SliderInt(_("##mod_enabled2LRSliderInt"), &mod_debugLR, 0, 2);
        ImGui::PopItemWidth();
        ImGui::Unindent(lineIndent);
    }
#endif
}
void PsychoMantis::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        if (!devil4_sdk::is_paused() && devil4_sdk::get_work_rate()->global_speed != 0.0f) {
            uint16_t lpower = 65535 / 14; // apparently 65535 is the limit but there isn't much difference if any from half the amount
            uint16_t rpower = 65535 / 14; // so we're divibing by double so its smoother
            if (PsychoMantis::_controller->isConnected()) {
                PsychoMantis::_controller->vibrate(
                    lpower * devil4_sdk::get_current_style_rank(), rpower * devil4_sdk::get_current_style_rank());
            }
            /* if (c->getState().Gamepad.wButtons & XINPUT_GAMEPAD_A) {
                c->vibrate(65535, 65535);
            }
            if (c->getState().Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
                c->vibrate(65535 / 2, 65535 / 2);
            }*/
        }
        if (mod_debugEnabled) {
            if (PsychoMantis::_controller->isConnected()) {
                switch (mod_debugLR) {
                case 0:
                    PsychoMantis::_controller->vibrate(mod_debugAmount, 0);
                    break;
                case 1:
                    PsychoMantis::_controller->vibrate(mod_debugAmount, mod_debugAmount);
                    break;
                case 2:
                    PsychoMantis::_controller->vibrate(0, mod_debugAmount);
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void PsychoMantis::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("psycho_mantis_toggle").value_or(false);
    psycho_mantis_slot_2 = cfg.get<bool>("psycho_mantis_slot_2").value_or(false);
    if (psycho_mantis_slot_2)
        PsychoMantis::Controller::_controllerNum = 1;
    else
        PsychoMantis::Controller::_controllerNum = 0;
}

void PsychoMantis::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("psycho_mantis_toggle", mod_enabled);
    cfg.set<bool>("psycho_mantis_slot_2", psycho_mantis_slot_2);
}
