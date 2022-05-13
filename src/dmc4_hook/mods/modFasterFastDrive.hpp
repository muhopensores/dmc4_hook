#pragma once

#include "../mod.hpp"

class FasterFastDrive : public Mod
{
public:
    FasterFastDrive() = default;

    static bool modEnabled;
    static bool easyFastDriveEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "FasterFastDrive"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch;
};
