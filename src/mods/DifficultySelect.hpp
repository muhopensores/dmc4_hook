#pragma once

#include "../mod.hpp"

class DifficultySelect : public Mod {
public:
    DifficultySelect() = default;

    // static bool modEnabled;

    void reset_patches();
    void set_default();
    void set_dmd();
    void set_gmd();

    std::string get_mod_name() override { return "DifficultySelect"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
	void on_gui_frame(int display) override;

    enum DIFF_IDX { def, dmd, gmd, DIFF_IDX_MAX };
    std::array<std::function<void(DifficultySelect*)>, 3> m_diffs;

    private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<Patch> patch3;
    std::unique_ptr<Patch> patch4;
    std::unique_ptr<Patch> patch5;
    std::unique_ptr<Patch> patch6;
    std::unique_ptr<Patch> patch7;
    std::unique_ptr<Patch> patch8;
    std::unique_ptr<Patch> patch9;
    std::unique_ptr<Patch> patch10;
    std::unique_ptr<Patch> patch11;
    std::unique_ptr<Patch> patch12;
    std::unique_ptr<Patch> patch13;
    std::unique_ptr<Patch> patch14;
    std::unique_ptr<Patch> patch15;
    std::unique_ptr<Patch> patch16;
    std::unique_ptr<Patch> patch17;
    std::unique_ptr<Patch> patch18;
    std::unique_ptr<Patch> patch19;
    std::unique_ptr<Patch> patch20;
    std::unique_ptr<Patch> patch21;
    std::unique_ptr<Patch> patch22;
    std::unique_ptr<Patch> patch23;
    std::unique_ptr<Patch> patch24;
    std::unique_ptr<Patch> patch25;
    std::unique_ptr<Patch> patch26;
    std::unique_ptr<Patch> patch27;
    std::unique_ptr<Patch> patch28;
    std::unique_ptr<Patch> patch29;
    std::unique_ptr<Patch> patch30;
    std::unique_ptr<Patch> patch31;
    std::unique_ptr<Patch> patch32;
    std::unique_ptr<Patch> patch33;
    std::unique_ptr<Patch> patch34;
    std::unique_ptr<Patch> patch35;
    std::unique_ptr<Patch> patch36;
    std::unique_ptr<Patch> patch37;
    std::unique_ptr<Patch> patch38;
    std::unique_ptr<Patch> patch39;
    std::unique_ptr<Patch> patch40;
    std::unique_ptr<Patch> patch41;
    std::unique_ptr<Patch> patch42;
    std::unique_ptr<Patch> patch43;
    std::unique_ptr<Patch> patch44;
    std::unique_ptr<Patch> patch45;
    std::unique_ptr<Patch> patch46;
    std::unique_ptr<Patch> patch47;
    std::unique_ptr<Patch> patch48;
    std::unique_ptr<Patch> patch49;
};
