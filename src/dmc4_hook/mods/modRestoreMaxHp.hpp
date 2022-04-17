#pragma once

#include "../mod.hpp"

class RestoreMaxHp : public Mod {
public:
    RestoreMaxHp() = default;

    static bool modEnabled;

    std::string getModName() override { return "RestoreMaxHp"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;
    void onFrame(fmilliseconds& dt) override;

private:
};

enum ENEMY_ID_NAMES : __int32 {
    SCARECROW_LEG = 0x0, // EM000
    SCARECROW_ARM = 0x1, // EM001
    EM002 = 0x2,
    SCARECROW_MEGA = 0x3, // EM003
    EM004 = 0x4,
    ANGELO_ALTO = 0x5, // EM005 (silver armour)
    ANGELO_BIANCO = 0x6, // EM006 (gold armour)
    EM007 = 0x7,
    MEPHISTO = 0x8, // EM008
    FAUST = 0x9, // EM009
    FROST = 0xA, // EM010
    ASSAULT = 0xB, // EM011
    BLITZ = 0xC, // EM012
    EM013 = 0xD,
    EM014 = 0xE,
    CUTLASS = 0xF, // EM015 (swimming sword)
    GLADIUS = 0x10, // EM016 (flying sword)
    BASILISK = 0x11, // EM017 (hellhound)
    EM018 = 0x12,
    EM019 = 0x13,
    EM020 = 0x14,
    EM021 = 0x15,
    EM022 = 0x16,
    EM023 = 0x17,
    EM024 = 0x18,
    EM025 = 0x19,
    EMDUMMY = 0x1A,
};
