#pragma once

#include "../mod.hpp"

class FastSprint : public Mod {
public:
    FastSprint() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "FastSprint"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch;
};
