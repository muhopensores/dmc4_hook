#pragma once

#include "../mod.hpp"

class PhotoMode : public Mod {
public:
    
    static bool mod_enabled;
    PhotoMode() = default;
    std::string get_mod_name() override { return "PhotoMode"; };
    //std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
};