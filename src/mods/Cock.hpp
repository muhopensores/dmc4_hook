#pragma once

#include "../mod.hpp"

class Cock : public Mod {
public:
    Cock() = default;
    std::string get_mod_name() override { return "Cock"; };

    void Cock::on_gui_frame(int display) override;
};