#pragma once

#include "../mod.hpp"

class PlayerTracker : public Mod {
public:
    PlayerTracker() = default;

    static uintptr_t jmp_return;
    static uPlayer* player_ptr;

    static Vector3f savedPlayerPosition;
    static float savedPlayerRotation;
    static int8_t savedPlayerWeight;
    static float savedPlayerInertia;
    static int8_t savedPlayerMoveBank;
    static int8_t savedPlayerMoveID;
    static uint8_t savedPlayerCancels1[4];
    static uint32_t savedPlayerCancels2[4];
    static uint32_t savedPlayerSword;
    static uint32_t savedPlayerGun;
    static uint32_t savedPlayerStyle;
    static uint32_t savedPlayerLockonAnimation;
    static uint32_t savedPlayerCanWeaponChange;

    static void SavePlayerXYZ();
    static void LoadPlayerXYZ();
    static void SavePlayerMove();
    static void LoadPlayerMove();

    std::string  get_mod_name() override { return "PlayerTracker"; };

    std::optional<std::string> on_initialize() override;
    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook;
};