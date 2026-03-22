#include "TestMod.hpp"
#include "sdk/Devil4.hpp" // include anything we need from other files

// anything between /*   */
// or after // is a comment and is not compiled

// make variables
static bool mod_enabled = false; // bools can only be 0 or 1
static int test_int = 0; // ints are whole numbers
static float test_float = 0.0f; // floats can have decimals

// functions do nothing unless they are called by something. This is called by the "Set DT Gauge" button in the gui section below
static void set_dt_gauge() {
    uPlayer* player = devil4_sdk::get_local_player(); // get the player
    if (player) {                // if the player exists,
        player->DT = test_float; // set the player's dt gauge to the value of test_float
        // (you can find more by typing player-> and scrolling the list, or check the definition of "uPlayer" by ctrl clicking it)
    }
}

// make something happen every frame
void TestMod::on_frame(fmilliseconds& dt) {
    /*uPlayer* player = devil4_sdk::get_local_player(); // get the player
    if (!player) { return; } // if the player does not exist, don't run the rest of this code
    if (mod_enabled) { // if mod_enabled is true
        // you could do something like:
        if (player->currentStyle == 4) { // if you enter darkslayer
            player->currentStyle = 3; // enter royal guard instead
        }
    }*/
}

// make something show on the gui
void TestMod::on_gui_frame(int display) {
    ImGui::Checkbox("Test Mod", &mod_enabled); // most imgui entries need a label and a pointer to the variable they change
    ImGui::SameLine(); // every imgui entry is on its own line by default, use SameLine to specify that the next entry should be on the same line
    help_marker("This is what test mod does!"); // draw a lil helpmarker (right click "help_marker" and hit "peek definition" to see exactly what this is doing)
    if (mod_enabled) {
        ImGui::Text("Example Text"); // some entries just need a label
        ImGui::SliderFloat("Test Float", &test_float, 0.0f, 10000.0f); // some entries need even more - if you hover the text "SliderFloat" you'll see it needs a label, variable, then min and max values
        if (ImGui::Button("Set DT Gauge")) { // buttons run the code in the if statement when you click them
            set_dt_gauge(); // make the button do the thing we defined in the function above. I could also just copy paste the code
                            // from the function here, but this way is cleaner and allows us to reuse the code if we want to make
                            // another button that does the same thing
        }
    }
}

// what happens on boot
std::optional<std::string> TestMod::on_initialize() {
    return Mod::on_initialize();
}

// what happens when the config is loaded
void TestMod::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("TestMod").value_or(false);
}

// what happens when you hit the save button
void TestMod::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("TestMod", mod_enabled);
}
