#pragma once

#include "../mod.hpp"

class CutsceneSkip : public Mod
{
public:
    CutsceneSkip() = default;

    static bool modEnabledIntro;
    static bool modEnabledOutro;

    void toggleIntro(bool enable);
    void toggleOutro(bool enable);

    std::string getModName() override { return "CutsceneSkip"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
};