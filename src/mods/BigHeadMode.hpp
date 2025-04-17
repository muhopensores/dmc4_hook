#pragma once

#include "../mod.hpp"

// Define ModName class that is a Mod
class BigHeadMode : public Mod {
public:
    // using default constructor
    BigHeadMode() = default;

    // override getModName() method with your mod name for logging.
    std::string get_mod_name() override { return "BigHeadMode"; };

    // override onInitialize() method with routines required to initialize the
    // mod. return Mod::onInitialize() on success or error string on failure.
    std::optional<std::string> on_initialize() override;

    /*void onFrame(fmilliseconds& dt) override;*/
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<FunctionHook> hook2;
};
