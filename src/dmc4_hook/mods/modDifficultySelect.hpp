#pragma once

#include "../mod.hpp"

class DifficultySelect : public Mod
{
public:
    DifficultySelect() = default;

    // static bool modEnabled;

    void ToggleFrostJumpout(bool enable);
    void setDefault();
    void setDMD();
    void setGMD();

    std::string getModName() override { return "DifficultySelect"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
    hl::Patch patch4;
    hl::Patch patch5;
    hl::Patch patch6;
    hl::Patch patch7;
    hl::Patch patch8;
    hl::Patch patch9;
    hl::Patch patch10;
    hl::Patch patch11;
    hl::Patch patch12;
    hl::Patch patch13;
    hl::Patch patch14;
    hl::Patch patch15;
    hl::Patch patch16;
    hl::Patch patch17;
    hl::Patch patch18;
    hl::Patch patch19;
    hl::Patch patch20;
    hl::Patch patch21;
    hl::Patch patch22;
    hl::Patch patch23;
    hl::Patch patch24;
    hl::Patch patch25;
    hl::Patch patch26;
    hl::Patch patch27;
    hl::Patch patch28;
    hl::Patch patch29;
    hl::Patch patch30;
    hl::Patch patch31;
    hl::Patch patch32;
    hl::Patch patch33;
    hl::Patch patch34;
    hl::Patch patch35;
    hl::Patch patch36;
    hl::Patch patch37;
    hl::Patch patch38;
    hl::Patch patch39;
    hl::Patch patch40;
    hl::Patch patch41;
    hl::Patch patch42;
    hl::Patch patch43;
    hl::Patch patch44;
    hl::Patch patch45;
    hl::Patch patch46;
    hl::Patch patch47;
    hl::Patch patch48;
    hl::Patch patch49;
    hl::Patch patchfrostjumpout;
};
