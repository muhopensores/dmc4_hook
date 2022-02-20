#pragma once

#include "../mod.hpp"

class CharacterSwap : public Mod
{
public:
    CharacterSwap() = default;

    static bool neroEnabled;
    static bool danteEnabled;

    void toggleNero(bool enable);
    void toggleDante(bool enable);

    std::string getModName() override { return "CharacterSwap"; };
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
};