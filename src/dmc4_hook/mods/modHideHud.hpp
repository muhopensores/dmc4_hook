#pragma once

#include "../mod.hpp"

class HideHud : public Mod
{
public:
    HideHud() = default;

    static bool modEnabledHealth;
    static bool modEnabledOrbs;
    static bool modEnabledStyle;
    static bool modEnabledTimer;
    static bool modEnabledBoey;

    void toggleHealth(bool enable);
    void toggleOrbs(bool enable);
    void toggleStyle(bool enable);
    void toggleTimer(bool enable);
    void toggleBoey(bool enable);

    std::string getModName() override { return "HideHud"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patchhealth;

    hl::Patch patchstyle;

    hl::Patch patchorbs1;
    hl::Patch patchorbs2;
    hl::Patch patchorbs3;
    hl::Patch patchorbs4;
    hl::Patch patchorbs5;
    hl::Patch patchorbs6;
    hl::Patch patchorbs7;
    hl::Patch patchorbs8;
    hl::Patch patchorbs9;
    hl::Patch patchorbs10;

    // timer
    hl::Patch patchbp;
    hl::Patch patchbpplus;
    hl::Patch patchm12;
    hl::Patch patchbpstageclearbonus1;
    hl::Patch patchbpstageclearbonus2;
    // hl::Patch patchm12flash;

    // boey
    hl::Patch patchboey01;
    hl::Patch patchboey02;
    hl::Patch patchboey03;
    hl::Patch patchboey04;
    hl::Patch patchboey05;
    hl::Patch patchboey06;
    hl::Patch patchboey07;
    hl::Patch patchboey08;
    hl::Patch patchboey09;
    hl::Patch patchboey10;
    hl::Patch patchboey11;
    hl::Patch patchboey12;
    hl::Patch patchboey13;
    hl::Patch patchboey14;
    // hl::Patch patchboey15;
    hl::Patch patchboey16;
    hl::Patch patchboey17;
    hl::Patch patchboey18;
    // hl::Patch patchboey19;
    hl::Patch patchboey20;
    hl::Patch patchboey21;
    hl::Patch patchboey22;
    hl::Patch patchboey23;
    hl::Patch patchboey24;
    hl::Patch patchboey25;
    hl::Patch patchboey26;
    hl::Patch patchboey27;
    hl::Patch patchboey28;
    hl::Patch patchboey29;
    hl::Patch patchboey30;
    hl::Patch patchboey31;
    hl::Patch patchboey32;
    hl::Patch patchboey33;
    hl::Patch patchboey34;
    hl::Patch patchboey35;
    hl::Patch patchboey36;
    hl::Patch patchboey37;
    hl::Patch patchboey38;
    hl::Patch patchboey39;
    hl::Patch patchboey40;
    hl::Patch patchboey41;
    hl::Patch patchboey42;
    hl::Patch patchboey43;
    hl::Patch patchboey44;
    hl::Patch patchboey45;
    hl::Patch patchboey46;
    hl::Patch patchboey47;
    hl::Patch patchboey48;
    hl::Patch patchboey49;
    hl::Patch patchboey50;
    hl::Patch patchboey51;
    hl::Patch patchboey52;
    hl::Patch patchboey53;
    hl::Patch patchboey54;
    hl::Patch patchboey55;
    hl::Patch patchboey56;
    hl::Patch patchboey57;
    hl::Patch patchboey58;
    hl::Patch patchboey59;
    hl::Patch patchboey60;
    hl::Patch patchboey61;
    hl::Patch patchboey62;
    hl::Patch patchboey63;
    hl::Patch patchboey64;
    hl::Patch patchboey65;
    hl::Patch patchboey66;
    hl::Patch patchboey67;
    hl::Patch patchboey68;
    hl::Patch patchboey69;
    hl::Patch patchboey70;
    hl::Patch patchboey71;
};
