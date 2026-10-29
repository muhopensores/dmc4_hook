#pragma once
// MoveDef: one player move read from an XML file (E:\DMC mod stuffs\Vibed\MoveDef\schema.md, v0.2; user guide
// docs\movedef.html). Parsing only;
// dev/movedef/MoveDef.cpp runs it. The subset of XML used: elements, attributes, comments. Numbers accept decimal or 0x hex.
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
    int16_t timer = 0;                      // frames allowed from this step to the NEXT (<= 0 = no limit), the game's meaning
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
    uint32_t cancel_words[5] = { 1, 0, 0, 0, 0x07000005 };   // cancelId[5]: word 0 set by attr at load (command C_COMMAND, gun
                                                             // C_GUN, melee C_NORM0), or C_ALL; as the game's own rows
    std::vector<Follows> follows;           // the string position: resolved to ids across the loaded files (MoveDef D22)
    std::vector<Window> windows;
};

struct Slot {                                           // <slot dir= block=>: the LMT block (in <lmt bank=>) per direction
    int dir = 0; int block = 0;
    float speed = -1.0f;                                // speed= / rise=: this direction's <launch> values (Gerb's up / down
    bool has_rise = false; float rise = 0.0f;           // jumps), < 0 / absent = the launch's
    bool has_roll = false; float roll = 0.0f;           // effect_roll=: the take-off effect's roll for this direction (the dive's 180)
    int damage_type_b = -1;                             // damage_type_b=: the <shell>s' reaction for this direction (-1 = the shell's)
};

struct AttackFields {                                   // kAttackStatus values given in the file, < 0 = not given
    int damage_type = -1, damage_type_b = -1, level_i = -1, level_b = -1, attack_id = -1, range_type = -1, hit_stop = -1,
        hitmark = -1, hit_se = -1, attack_flag = -1, blown_angle = -1, element = -1;
    float damage = -1.0f, stylish_point = -1.0f, stylish_timer = -1.0f, dt_add = -1.0f;
};

struct Attack : AttackFields {                          // <attack name= [atk= entry=] .../> under <move> (D25): attack data for
    std::string name;                                   // the file's <shell attack=>s; without atk= / entry= it is defined by
    std::string atk;                                    // its values alone (the rest from kDefaultAttack, MoveDefShell.cpp),
    int entry = -1;                                     // with them it is that .atk entry with the values as overrides
    std::string entry_name;                             // entry="Dropkick": the entry by its mAsName
};

struct Shell : AttackFields {                           // <shell>: a code-defined attack capsule riding the player (D18, GerbShell's recipe)
    bool in_launch = false;                             // inside <launch>: spawned at the launch, along its direction
    float frame = 0.0f;                                 // in a <stage>: the motion frame it spawns at (once per entry)
    char atk[128] = "";                                 // rAttackStatusData path without extension (default Collision\pl000 / pl006)
    int entry = 0;                                      // the .atk entry copied into the group
    char entry_name[16] = "";                           // entry="Dropkick": the entry by its mAsName instead of its index
    std::string attack;                                 // attack="name": a <attack> of the file (resolved at load)
    bool defined = false;                               // the attack is defined in the file: no entry is read
    char as_name[16] = "";                              // its name (mAsName, which the style display shows)
    bool explicit_file = false;                         // atk= / entry= given on the shell itself
    int kind = 1;                                       // cCollisionGroup::mKind (1 = a player shell)
    uint32_t vsattr = 0;                                // 0 = the game's table for the kind
    int unit_line = 0x12;
    float radius = 60.0f, length = 120.0f, height = 80.0f, life = 90.0f;   // cm, cm along the direction, cm above the feet, ticks
    int knock = 0;                                      // 0 direction (ANG along the move's direction), 1 facing (ANG), 2 away (POS)
    float knock_angle = 0.0f;                           // degrees added to the knock yaw (0 / 1)
    bool one_hit = false, show = false;
    // the AttackFields are the kAttackStatus overrides, < 0 = the entry's (or the <attack>'s)
};

struct Cancel {                                         // <cancel frame= kind= categories=>
    float frame = 0.0f;
    uint8_t mode = 0x10;                                // 0x10 open, 0x20 close, 0x30 buffer
    std::vector<int> categories;                        // CANCEL_NO values; empty = every category
};

struct Trigger {                                        // <trigger byte= value= frame=>: a .ctn contact byte (set 0) for <next on="contact">
    float frame = 0.0f;
    int byte = 4;                                       // info index: 0 atck, 1 free2, 2 camVib, 3 free1, 4 free0, 5 padVib
    uint8_t value = 1;
};

struct Next {                                           // <next ... to=>: a stage transition, checked in order every tick
    std::string to;                                     // "exit", or a move name (same character; "move:name" accepted)
    float frame = -1.0f;                                // frame="N": the motion frame has reached N (a looping block wraps: N past its end never fires)
    int ticks = -1;                                     // ticks="N": N ticks since the stage started (overlays: since its motion started)
    bool motion_end = false;                            // on="motion_end"
    bool landing = false;                               // on="landing": vel.y <= 0 and ground contact
    bool always = false;                                // on="always"
    int byte = -1; int value = 0;                       // byte="free0" value="2": that contact byte equals value
    uint32_t button = 0; bool held = false;             // button="melee" [held="true"]: pressed this tick, or held
    int idle = -1;                                      // button= with idle="N": that button has NOT been pressed for N ticks (a mash that stopped)
    bool stick_neutral = false;                         // stick="neutral": mAnlgPow == 0
};

struct FxEvent {                                        // <effect> / <se> / <kill_effect> / <finish_effect> in a <stage>: fired by frame
    enum Kind { EFFECT = 0, SE = 1, KILL = 2, FINISH = 3 } kind = EFFECT;
    float frame = 0.0f;
    bool repeat = false;                                // repeat="loop": re-armed when the motion frame wraps (a looping block)
    int list = -1, index = 0;                           // EFFECT by list / index of the effect table ...
    std::string efl;                                    // ... or by .efl path (effect_generator)
    std::string joint = "self";                         // self | weapon (the held weapon unit)
    int32_t group = -1;                                 // spawn masks: -1 = all
    int32_t material = -1;                              // -1 = all, -2 = "ground" (getEfctMtrlFlg under the player)
    int unit_line = 0x14;
    int type = 0, id = -1;                              // uPlayer::setEffect registration (id -1 = unnamed); KILL / FINISH: the id
};

struct Fx {                                             // <effect stay> / <se> inside <launch>
    bool effect = false; int list = 0, index = 0; float tilt = 0.0f, roll = 0.0f;
    bool se = false; int se_id = 0;
};

struct Launch {                                         // <launch>: the ballistic jump (Gerb / DanteAirDodge)
    bool present = false;
    float frame = 0.0f;
    std::string direction = "stick";                    // stick | facing | vertical (up on the ground, down in the air)
    float speed = 0.0f, rise = 0.0f, gravity = 0.0f, max_fall = 30.0f, side_angle = 50.0f;
    int face = 0;                                       // face="true" 1: turn toward forward / left / right (a back jump keeps
                                                        // facing, as Gerb's); "all" 2: back too; "false" 0
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
    int motion = -1;                                    // motion="bank:slot" as a word, when no <slot>s (a vanilla motion)
    int block = -1;                                     // block="N": block N of the move's own <lmt>, when no <slot>s
    int ctn_bank = -1, ctn_slot = -1;                   // ctn="bank:slot": the vanilla .ctn script borrowed for the motion
    uint32_t attr = 1;                                  // the request's attribute word (1 = A_NULL_TRANS_OFF)
    float interp = 4.0f, start = 0.0f, speed = 1.0f;
    bool stop = false;                                  // stop="true": zero the velocity / acceleration at entry (a stationary move)
    bool status_jump = false;                           // status="jump": set AST_JUMP at the launch
    std::string land = "exit";                          // what a landing does: exit (setActionLand) | none
    bool enemy_step = true;                             // the enemy step can interrupt from frame 0 (checkEmJumpBegin)
    uint32_t eat_press = 0;                             // eat_press="jump": button bits eaten until released after entry
    int watchdog = 60;                                  // ticks without motion progress before the move bails out
    std::vector<Slot> slots;
    std::vector<Cancel> cancels;                        // sorted by frame at load
    std::vector<Trigger> triggers;                      // sorted by frame at load
    std::vector<Next> nexts;
    std::vector<FxEvent> fx;                            // the runtime timeline, sorted by frame at load
    Launch launch;
    AirJump air_jump;
    std::vector<Shell> shells;                          // in the stage and in its <launch>
};

struct Lmt { int bank = -1; std::string path; };        // <lmt path= [bank=]>: no bank = placed by the motion registry
                                                        // (sdk/MotionRegistry.hpp); bank 10-14 = loaded into that bank

struct File {
    std::string source;                                 // the file path
    bool commands_only = false;                         // a <commands character=> file: shared <command>s, no move
    bool overlay = false;                               // overlay="true": `id` is a VANILLA move; the stage's <next>s run on it (no row, no motion)
    std::vector<uint32_t> extra_ids;                    // ids="28 29": further vanilla ids an overlay covers (a move with several rows)
    std::vector<Command> shared_commands;               // its <command>s (also any extra <command>s of a move file)
    std::string name;
    int character = -1;                                 // 0 Nero (pl000), 1 Dante (pl006)
    uint32_t id = 0;                                    // atckId
    int version = 2;
    Lmt lmt;
    Table table;
    Command command;
    std::vector<Stage> stages;
    std::vector<Attack> attacks;                        // its <attack>s (D25)
};

bool load(const char* path, File& out);                 // false + spdlog::error on a missing / malformed file

// name tables shared with the runner
uint32_t button_bits(const char* name);                 // melee 1, gun 2, jump 4, lockon 0x10, style 0x200, or a number
int cancel_category(const char* name);                  // C_NORM0 .. C_ALL, -1 unknown
int contact_byte(const char* name);                     // atck / free2 / camVib / free1 / free0 / padVib -> info index, -1 unknown

} // namespace xmlmove
