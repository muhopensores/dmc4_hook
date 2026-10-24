#pragma once
// MoveDef: one player move read from an XML file (E:\DMC mod stuffs\Vibed\MoveDef\schema.md, v0.2). Parsing only;
// mods/MoveDef.cpp runs it. The subset of XML used: elements, attributes, comments. Numbers accept decimal or 0x hex.
#include <cstdint>
#include <string>
#include <vector>

namespace xmlmove {

// directions a <slot> / <launch> can name, Gerb's order (= its LMT slots)
enum Dir : int { FORWARD = 0, DOWN = 1, BACK = 2, LEFT = 3, RIGHT = 4, UP = 5, DIR_COUNT = 6 };
int dir_from_name(const char* name);        // -1 unknown
const char* dir_name(int dir);

struct CommandStep {                        // one <btn> / <lever> of a <command>: the kCmdData fields
    int type = 1;                           // 1 BTN, 2 LEVER
    uint32_t btn = 0;                       // BTN: the button bits
    uint16_t mode = 1;                      // BTN: 1 trg / 2 rel / 3 on; LEVER: 1 target 45 / 2 target 70 / 3 screen
    uint16_t lever = 0;                     // LEVER: 0 centred / 1 forward / 4 back / 15 any
    int16_t timer = 0;                      // frames allowed to reach this step (0 = this tick, -1 = no limit)
};

struct Command {                            // <command name= condition= predicate= active=>: an extra input command (MoveDef D17)
    bool present = false;
    std::string name;
    float side_angle = 50.0f;               // for predicate="stick_sideways" on a shared command (a move's own uses its <launch>)
    uint32_t condition = 0;                 // input_commands::CND_TYPE value (0 none, 1 lock-on, 2 dash, 3 laser)
    std::string predicate;                  // a runner-side per-tick check by name ("stick_sideways"), empty = none
    int16_t active = 1;
    std::vector<CommandStep> steps;
};

struct Follows { std::string move; int window = -1; bool any = false; };   // <follows move= window= or=>: a string step after `move`
struct Window  { int category = -1; bool any = false; };                   // <window category= or=>: a cancel window that must be open

struct Table {                              // <table>: the kAtckDefTbl row
    bool present = false;
    uint32_t attr = 2;                      // 0 melee, 1 gun, 2 command, 4 proper
    uint32_t level = 7;
    bool start = true, cancel = false, repeat = false;   // atckInfo bits 1 / 2 / 4
    std::string command;                    // <input command=>: a <command>'s name, or a CMD_ID number; empty = the attr's button
    int32_t weapon = -1, style = -1;
    uint32_t etc = 0;                       // atckConditionEtc (word 7)
    uint32_t air = 2;                       // atckAs: 1 ground, 2 air, 3 both, 0x40 fly
    bool explicit_words = false;            // cancel_words="..." given: used as they are
    uint32_t cancel_words[5] = { 1, 0, 0, 0, 0x07000005 };   // cancelId[5] as the hand-coded rows carry them (C_NORM0 open, or C_ALL open)
    std::vector<Follows> follows;           // the string position: resolved to ids across the loaded files (MoveDef D22)
    std::vector<Window> windows;
};

struct Slot { int dir = 0; int block = 0; };            // <slot dir= block=>: the LMT block (in <lmt bank=>) per direction

struct Cancel {                                         // <cancel frame= kind= categories=>
    float frame = 0.0f;
    uint8_t mode = 0x10;                                // 0x10 open, 0x20 close, 0x30 buffer
    std::vector<int> categories;                        // CANCEL_NO values; empty = every category
};

struct Fx {                                             // <effect stay> / <se> inside <launch>
    bool effect = false; int list = 0, index = 0; float tilt = 0.0f, roll = 0.0f;
    bool se = false; int se_id = 0;
};

struct Launch {                                         // <launch>: the ballistic jump (Gerb / DanteAirDodge)
    bool present = false;
    float frame = 0.0f;
    std::string direction = "stick";                    // stick | facing
    float speed = 0.0f, rise = 0.0f, gravity = 0.0f, max_fall = 30.0f, side_angle = 50.0f;
    bool face = false;
    Fx fx;
};

struct AirJump {                                        // <air_jump cost= required= suppress_air_hike= per_air=>
    int cost = 0;                                       // mWJumpCnt += cost at entry
    bool required = false;                              // the move needs an air jump left to start
    bool suppress_air_hike = false;                     // the air hike steps aside for this move's jump press
    int per_air = 0;                                    // starts between landings, 0 = no cap
};

struct Stage {
    std::string name;
    int motion = -1;                                    // motion="bank:slot" as a word, when no <slot>s
    int ctn_bank = -1, ctn_slot = -1;                   // ctn="bank:slot": the vanilla .ctn script borrowed for the motion
    uint32_t attr = 1;                                  // the request's attribute word (1 = A_NULL_TRANS_OFF)
    float interp = 4.0f, start = 0.0f, speed = 1.0f;
    bool status_jump = false;                           // status="jump": set AST_JUMP at the launch
    std::string land = "exit";                          // what a landing does: exit (setActionLand) | none
    bool enemy_step = true;                             // the enemy step can interrupt from frame 0 (checkEmJumpBegin)
    uint32_t eat_press = 0;                             // eat_press="jump": button bits eaten until released after entry
    int watchdog = 60;                                  // ticks without motion progress before the move bails out
    std::vector<Slot> slots;
    std::vector<Cancel> cancels;                        // sorted by frame at load
    Launch launch;
    AirJump air_jump;
};

struct Lmt { int bank = -1; std::string path; };        // <lmt bank= path=>: loaded per player object

struct File {
    std::string source;                                 // the file path
    bool commands_only = false;                         // a <commands character=> file: shared <command>s, no move
    std::vector<Command> shared_commands;               // its <command>s (also any extra <command>s of a move file)
    std::string name;
    int character = -1;                                 // 0 Nero (pl000), 1 Dante (pl006)
    uint32_t id = 0;                                    // atckId
    int version = 2;
    Lmt lmt;
    Table table;
    Command command;
    std::vector<Stage> stages;
};

bool load(const char* path, File& out);                 // false + spdlog::error on a missing / malformed file

// name tables shared with the runner
uint32_t button_bits(const char* name);                 // melee 1, gun 2, jump 4, lockon 0x10, style 0x200, or a number
int cancel_category(const char* name);                  // C_NORM0 .. C_ALL, -1 unknown

} // namespace xmlmove
