#pragma once

#include "../mod.hpp"

class RayTest : public Mod {
public:
    RayTest() = default;
    std::string get_mod_name() override { return "RayTest"; };
    void on_gui_frame(int display) override;
};