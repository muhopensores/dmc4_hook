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

static void PlayRollAnimation() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    player->movePart = 3;
    devil4_sdk::indexed_anim_call(8, player, 0, 1.0f, 0.0f, 3.0f);
    player->playMoveOnce = 4;
}

void TestMod::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        ImGui::Begin("AerialWindow");
        if (ImGui::Button("Play Roll Animation")) {
            PlayRollAnimation();
        }
        static bool rollSpam = false;
        ImGui::Checkbox("rollSpam", &rollSpam);
        if (rollSpam) {
            PlayRollAnimation();
        }
        ImGui::End();
    }
}

struct AnimationInfo {
    int moveID;               // 4 bytes
    float animFrame;          // another 4 bytes, so now the size of AnimationInfo is 8 bytes
    float animFrameMax;       // another 4 bytes, so now the size of AnimationInfo is 12 bytes
};

// size: 12 bytes

struct Player {
    int playerID;             // 4 bytes
    int HP;                   // 4 bytes (= 8 bytes so far)
    int HPMax;                // 4 bytes (= 12 bytes so far)
    int moveID;               // 4 bytes (= 16 bytes so far)
    AnimationInfo* animInfo1; // 4 bytes because its a * (pointer) (= 20 bytes so far)
};

// size: 20 bytes

   //AnimationInfo animInfo2;  // 12 bytes becaus its not a pointer (= 32 bytes so far)
   // size: 32 bytes

void example() {
    Player* dante = (Player*)0xB00B135;
    dante->HP = 100; // set dante's HP to 100
    dante->animInfo1->animFrame = 5.0f;
}
// make something show on the gui
void TestMod::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox("Test Mod", &mod_enabled);
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
