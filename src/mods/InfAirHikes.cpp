#include "InfAirHikes.hpp"

bool InfAirHikes::mod_enabled{ false };

std::optional<std::string> InfAirHikes::on_initialize() {
    MutatorRegistry::define("InfiniteAirHikes")
        .description("hehe")
        .on_init([&] {toggle(true); })
        .set_timer(15.0, [&] { toggle(false); });
    return Mod::on_initialize();
}

void InfAirHikes::toggle(bool enable) {
    if (enable){
        install_patch_offset(0x03ACA3A, patch1, "\x88\x91\x7E\x1E\x00\x00", 6);     // set air hike count 0 rather than wall hike 0 every air hike
        install_patch_offset(0x03ACA51, patch2, "\x90\x90\x90\x90\x90\x90\x90", 7); // wall hike add to jump count
        install_patch_offset(0x03ACA4A, patch3, "\x90\x90\x90\x90\x90\x90\x90", 7); // kills mov 0 to can wall hike
    }
    else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
    }
}

void InfAirHikes::on_gui_frame() {
    if (ImGui::Checkbox("Infinite Air Hikes", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfAirHikes::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_air_hikes").value_or(false);
    toggle(mod_enabled);
};

void InfAirHikes::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_air_hikes", mod_enabled);
};
