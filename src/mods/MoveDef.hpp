#pragma once

#include "../mod.hpp"

// MoveDef: player moves defined in XML files (E:\DMC mod stuffs\Vibed\MoveDef\: design.md, schema.md v0.2), run by a
// generic state function. It is the recipe Gerb and DanteAirDodge settled (design.md 5b), data-driven:
//   - every nativePC\movedef\*.xml is loaded at start-up: its <table> becomes a kAtckDefTbl row (MoveTable::add_row),
//     its id goes into sdk/MoveRegistry (ID gate + dispatch), its <command> into sdk/InputCommand (a free mCmdFlag bit),
//     its <lmt> is loaded into a free motion bank for every player object that enters the move;
//   - the state function (movedef::run_move) plays the stage: direction from the stick, the motion from the matching
//     <slot>, the borrowed .ctn script then a synthesised one (sdk/CtnScript) with the cancel gate, the <launch>
//     ballistics with the take-off effect / SE, the landing exit, the watchdog, the press hygiene and the enemy step;
//   - the air hike steps aside for a move whose command uses the jump button (hook on uPlayer::checkWJumpBegin
//     0x803880, the base both characters' overrides call; the C++ dodge hooks Dante's override, a different address).
// Reload re-reads the files for the moves already loaded (tunables); a new file needs a restart (its row must exist
// before the game hands the table to a player).
namespace movedef {

extern bool mod_enabled;
extern uintptr_t jmp_ret_wjump;      // 0x803886, after the 6 prologue bytes the air-hike hook replaces
extern char folder[128];             // where the .xml files are, relative to the game folder

void load_all();                     // at start-up
void reload();                       // re-read the loaded moves' files in place
void apply_rows();                   // the rows' enable bits after a toggle
void write_example();                // example.xml in the folder (its move commented out), unless it exists
void __stdcall run_move(uintptr_t player, uint32_t atck_id);   // the registry's state function for every XML move

} // namespace movedef

class MoveDef : public Mod {
public:
    MoveDef() = default;
    std::string get_mod_name() override { return "MoveDef"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

    std::unique_ptr<FunctionHook> hook_wjump, hook_cmd;
};
