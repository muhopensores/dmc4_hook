#include "Mod.hpp"

class MessageDisplayMod : public Mod
{
public:
    // using default constructor
    MessageDisplayMod() = default;
    Mod::ModType get_mod_type() override { return SLOW; }

    // override getModName() method with your mod name for logging.
    std::string get_mod_name() override { return "MessageDisplayMod"; };

    // override onInitialize() method with routines required to initialize the mod.
    // return Mod::onInitialize() on success or error string on failure.
    std::optional<std::string> on_initialize() override;


    void custom_imgui_window();
    //void onFrame(fmilliseconds& dt) override;
    void on_gui_frame() override;
    //void onTwitchCommand(std::size_t hash) override;
    //void onUpdateInput(hl::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
