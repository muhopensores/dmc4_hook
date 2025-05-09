#include "PsychoMantis.hpp"
#include "../sdk/Devil4.hpp"

// https://gist.github.com/stroum
bool PsychoMantis::mod_enabled = false;
bool PsychoMantis::psycho_mantis_slot_2 = false;
bool PsychoMantis::mod_debugEnabled = false;
int PsychoMantis::mod_debugAmount = 0;
int PsychoMantis::mod_debugLR = 0;
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

void PsychoMantis::on_gui_frame(int display) {
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Stylevibe"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Put your controller on the floor. Put it down as flat as you can..."
        "that's good. Now I will move your controller by the power of my style alone!!\n"
        "(Vibrates your controller more the higher your style rank)"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        if (ImGui::Checkbox(_("Vibe Controller 2 Instead"), &psycho_mantis_slot_2)) {
            if (psycho_mantis_slot_2) {
                PsychoMantis::Controller::_controllerNum = 1;
            }
            else
                PsychoMantis::Controller::_controllerNum = 0;
        }
        ImGui::SameLine();
        help_marker(_("One simple trick to get your gf interested in your dmc combos"));
        ImGui::Unindent(lineIndent);
    }
#ifndef NDEBUG
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("...debug"), &mod_debugEnabled);
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
    ImGui::EndGroup();
}
void PsychoMantis::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        if (!devil4_sdk::is_paused() && devil4_sdk::get_work_rate()->global_speed != 0.0f) {
            constexpr uint16_t maxPower = UINT16_MAX;
            constexpr uint16_t halfPower = UINT16_MAX / 2;
    
            uint16_t styleRank = devil4_sdk::get_stylish_count()->current_style_tier;
            uint16_t finalPower;
            if (styleRank < 5)
                finalPower = (halfPower * styleRank) / 4; // higher values are more gradual
            else
                finalPower = maxPower;

            if (PsychoMantis::_controller->isConnected()) {
                PsychoMantis::_controller->vibrate(finalPower, 0);
            }
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
    if (psycho_mantis_slot_2) PsychoMantis::Controller::_controllerNum = 1;
}

void PsychoMantis::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("psycho_mantis_toggle", mod_enabled);
    cfg.set<bool>("psycho_mantis_slot_2", psycho_mantis_slot_2);
}
