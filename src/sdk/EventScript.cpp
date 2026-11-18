#include "EventScript.hpp"
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Sizes: SE cEventScript::skipCode 0x590BF0 / skipCondition 0x590CE0 (every DX9 room script parses with them, 153 of
// 153). Names: conditions from the PDB enum EVENT_MACRO::BRANCH_CONDISION, control codes from
// cBranchScript::CONTROL_CODE_ENUM; opcodes have no PDB names (macros), so these describe what each one does
// (Vibed/RE/event_script.md), and only opcodes whose operands are traced get typed operands.

namespace event_script {
namespace {

struct SizeGroup { int size; std::initializer_list<int> codes; };

struct Tables {
    uint8_t code[256] = {};
    uint8_t cond[256] = {};
    Tables() {
        const SizeGroup codes[] = {
            {4, {0x00, 0x15, 0x29, 0x2A, 0x44, 0x76}},
            {3, {0x01, 0x02, 0x03, 0x05, 0x09, 0x0C, 0x11, 0x12, 0x1A, 0x1C, 0x1F, 0x20, 0x23, 0x24, 0x27, 0x28, 0x2B, 0x2E,
                 0x3B, 0x3D, 0x40, 0x4D, 0x5E, 0x6C, 0x74, 0x75, 0x7E}},
            {2, {0x04, 0x06, 0x0A, 0x0B, 0x0E, 0x0F, 0x10, 0x13, 0x16, 0x17, 0x18, 0x19, 0x1D, 0x21, 0x22, 0x25, 0x26, 0x2C,
                 0x2F, 0x30, 0x32, 0x33, 0x3F, 0x41, 0x43, 0x45, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x4E, 0x4F, 0x51, 0x52, 0x53,
                 0x54, 0x55, 0x58, 0x5B, 0x5C, 0x60, 0x63, 0x64, 0x69, 0x6A, 0x6B, 0x6D, 0x6E, 0x6F, 0x73, 0x7C, 0x7D}},
            {1, {0x07, 0x0D, 0x14, 0x1E, 0x2D, 0x31, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3C, 0x3E, 0x42, 0x46, 0x49,
                 0x50, 0x56, 0x57, 0x59, 0x5A, 0x5D, 0x5F, 0x62, 0x65, 0x66, 0x67, 0x68, 0x70, 0x71, 0x77, 0x78, 0x7B}},
            {17, {0x08, 0x7A}}, {14, {0x1B}}, {5, {0x61, 0x79}}, {25, {0x72}},
        };
        for (const auto& g : codes) for (int c : g.codes) code[c] = (uint8_t)g.size;
        const SizeGroup conds[] = {
            {2, {0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x10, 0x11, 0x12, 0x13, 0x19, 0x1A,
                 0x1F, 0x20, 0x23, 0x25, 0x26, 0x27, 0x28, 0x2B}},
            {3, {0x01, 0x0E, 0x14, 0x17, 0x1E, 0x2E}},
            {1, {0x08, 0x0F, 0x15, 0x16, 0x18, 0x1B, 0x1C, 0x1D, 0x21, 0x22, 0x24, 0x2A, 0x2C, 0x2D, 0x2F, 0x30}},
            {5, {0x29}},
        };
        for (const auto& g : conds) for (int c : g.codes) cond[c] = (uint8_t)g.size;
    }
};
const Tables& tables() { static const Tables t; return t; }

// Operand formats: b u8, s s8, w u16, d u32, f f32 (little endian, packed).
struct Named { int code; const char* name; const char* fmt; };

const Named kOps[] = {
    {0x00, "ROOM_CHANGE", "wb"},        // room, door (aGame::setRoomChange)
    {0x01, "ROOM_FLAG", "bb"},          // set 1 / clear 0, bit
    {0x02, "MISSION_FLAG", "bb"},       // set 1 / clear 0, bit
    {0x03, "PRELOAD_ROOM", "w"},        // room (aMission::loadNextRoom)
    {0x04, "ENEMY_SET", "b"},           // enemy group (uEnemySetCtrl)
    {0x05, "DEMO", "bb"},               // cutscene, mode (aRoom::setDemo)
    {0x06, "SEAL", "b"},                // doors sealed 1 / 0
    {0x07, "WAIT_DEMO", ""},
    {0x08, "SET_PLAYER", "ffff"},       // x, y, z, angle in degrees
    {0x09, "PLACEMENT", "bb"},          // setup mode, file index (.pla)
    {0x0A, "STAGESET", "b"},
    {0x0B, "CAMERA", "b"},              // 1 reset, 0 resume
    {0x0C, "WAIT", "w"},                // frames; follow with WAIT_TICK
    {0x0D, "WAIT_TICK", ""},
    {0x0E, "REQUEST", "b"},             // 0 pause menu, 0x32 tutorial, 0x64 power up, 0xC8-0xCA secret mission
    {0x0F, "FADE_EVENT", "b"},
    {0x10, "BGM", "b"},                 // 0 fade the room sound, else a stream request
    {0x12, "SCHEDULER_TRANS", "bb"},    // file index (scheduler name), on
    {0x15, "MESSAGE", "wb"},
    {0x16, "PLACE_PLAYER", "s"},        // .evh spawn point; 0x80 | n = hit volume n; -1 here; -2 last landing point
    {0x17, "BOSS_END", "b"},
    {0x18, "GET_ITEM", "b"},
    {0x1C, "STAGESET_SWITCH", "bb"},
    {0x1F, "COUNTER_SET", "bb"},        // counter, value
    {0x20, "CAMERA_AREA", "bb"},        // on, area
    {0x22, "EVENT_BATTLE", "b"},
    {0x25, "COLLISION", "b"},           // file index (.sbc)
    {0x27, "SAVE_FLAG", "bb"},
    {0x2C, "ENEMY_GENOCIDE", "b"},
    {0x2F, "PLAYER_CONTROL", "b"},      // 1 take control away, 0 give it back
    {0x33, "COUNTER_INC", "b"},
    {0x37, "WAIT_FADE", ""},
    {0x3B, "SCHEDULER_MOVE", "bb"},
    {0x45, "FADE", "b"},
    {0x49, "GAME_OVER", ""},
    {0x4B, "EVENT_ONLY", "s"},          // > 0 / < 0 enter, 0 leave
    {0x51, "ENEMY_GROUP_LOAD", "b"},    // 0xFF = release the room's enemy arcs
    {0x52, "LOAD_ARC", "b"},            // file index
    {0x54, "TRANS", "b"},
    {0x58, "SCRIPT_PLAYER", "b"},
};

const char* const kConds[0x31] = {
    "CK_PL_IN_BOX", "CK_ROOM_NO", "CK_EMPTY_EM", "CK_ROOM_FLAG", "CK_MISSION_FLAG", "CK_MISSION_NO", "CK_PL_ENT_BOX",
    "CK_PL_ESC_BOX", "CK_SEALING", "CK_DOOR", "CK_ENTER_DOOR", "CK_PL_CK_BOX", "CK_WARP", "CK_PLAYER_ID",
    "CK_EVENT_FREE", "CK_BOSS_DEAD", "CK_MOVE_STATUE", "CK_START_STATUE", "CK_GAME_FLAG", "CK_COMBO_BREAK",
    "CK_STATUE_BOX", "CK_STINGER_JUMP", "CK_PL_OUT_CTRL", "CK_EM_IN_BOX", "CK_EM_COUNT", "CK_PL_ENT_CAMERA",
    "CK_PL_ESC_CAMERA", "CK_EM021_DRAGON_WAIT", "CK_EM019_BAEL_CHANGE", "CK_SECRET_TIME_UP", "CK_STATUE_ENT_BOX",
    "CK_GET_BLUE_ORB", "CK_SET_EXIST", "CK_BOX_ALL_OPEN", "CK_TUTORIAL_ON", "CK_CLEAR_MISSION", "CK_PAUSE_GAME",
    "CK_RANDOM_100", "CK_DIFFICULTY", "CK_GAME_MODE", "CK_GOD_TURN", "CK_ELAPSE_TIME", "CK_PL_EXIST",
    "CK_PL_IN_CAMERA", "CK_DISABLE_TIMER", "CK_BENCHMARK_LOOP", "CK_DEMO_FRAME", "CK_EMPTY_ORB", "CK_PL_CHANGED",
};

const char* cond_fmt(int c) {
    switch (c) {
    case 0x01: return "w";      // CK_ROOM_NO
    case 0x0E: return "bb";     // CK_EVENT_FREE counter, value
    case 0x29: return "d";      // CK_ELAPSE_TIME
    case 0x2E: return "w";      // CK_DEMO_FRAME
    default: return nullptr;    // raw bytes
    }
}

enum : uint8_t { IF = 0xF0, NIF, AND_IF, AND_NIF, OR_IF, OR_NIF, DEFAULT_, ELSE_, END_IF, BREAK_, SWITCH_, END_SWITCH,
                 END_DATA = 0xFF };
const char* const kCtrl[16] = {"IF", "NIF", "AND_IF", "AND_NIF", "OR_IF", "OR_NIF", "DEFAULT", "ELSE", "END_IF", "BREAK",
                               "SWITCH", "END_SWITCH", nullptr, nullptr, nullptr, "END"};

const Named* op_named(int code) {
    for (const Named& n : kOps) if (n.code == code) return &n;
    return nullptr;
}

std::string raw_fmt(int n) { return std::string(n > 0 ? n : 0, 'b'); }

std::string op_fmt(int code) {
    if (const Named* n = op_named(code)) return n->fmt;
    return raw_fmt(tables().code[code] - 1);
}

std::string condition_fmt(int c) {
    if (const char* f = cond_fmt(c)) return f;
    return raw_fmt(tables().cond[c] - 1);
}

int fmt_bytes(const std::string& f) {
    int n = 0;
    for (char ch : f) n += (ch == 'w') ? 2 : (ch == 'd' || ch == 'f') ? 4 : 1;
    return n;
}

void append_operands(std::string& s, const uint8_t* p, const std::string& f) {
    char buf[48];
    for (char ch : f) {
        switch (ch) {
        case 'b': std::snprintf(buf, sizeof buf, " %u", (unsigned)p[0]); p += 1; break;
        case 's': std::snprintf(buf, sizeof buf, " %d", (int)(int8_t)p[0]); p += 1; break;
        case 'w': std::snprintf(buf, sizeof buf, " %u", (unsigned)(p[0] | p[1] << 8)); p += 2; break;
        case 'd': { uint32_t v; std::memcpy(&v, p, 4); std::snprintf(buf, sizeof buf, " %u", v); p += 4; break; }
        case 'f': { float v; std::memcpy(&v, p, 4); std::snprintf(buf, sizeof buf, " %.9g", v); p += 4; break; }
        default: buf[0] = 0; break;
        }
        s += buf;
    }
}

std::string hex2(int v) { char b[8]; std::snprintf(b, sizeof b, "%02X", v); return b; }

std::string cond_name(int c) { return c <= 0x30 ? kConds[c] : "C_" + hex2(c); }
std::string op_name(int code) {
    if (const Named* n = op_named(code)) return n->name;
    return "OP_" + hex2(code);
}

// One instruction at p: its size, 0 if bad (error set).
int instruction_size(const uint8_t* p, size_t left, std::string* error) {
    const uint8_t b = p[0];
    if (b >= 0xF0) {
        if (b <= OR_NIF) {
            if (left < 2) { if (error) *error = "truncated condition"; return 0; }
            const int c = tables().cond[p[1]];
            if (!c) { if (error) *error = "unknown condition 0x" + hex2(p[1]); return 0; }
            if ((size_t)(1 + c) > left) { if (error) *error = "truncated condition"; return 0; }
            return 1 + c;
        }
        if (!kCtrl[b - 0xF0]) { if (error) *error = "unknown control code 0x" + hex2(b); return 0; }
        return 1;
    }
    const int n = tables().code[b];
    if (!n || b > kMaxDx9Code) { if (error) *error = "unknown or SE-only opcode 0x" + hex2(b); return 0; }
    if ((size_t)n > left) { if (error) *error = "truncated opcode 0x" + hex2(b); return 0; }
    return n;
}

// The source structure: IF / NIF / SWITCH / DEFAULT open a block; END_IF, END_SWITCH and BREAK close one (a SWITCH's
// cases are IF <condition> ... BREAK and DEFAULT ... BREAK, without END_IF: aRoom000's per-frame script and three
// others; cBranchScript::skipBranch counts DEFAULT and SWITCH as openers too).
int depth_change(uint8_t b) {
    if (b == IF || b == NIF || b == SWITCH_ || b == DEFAULT_) return 1;
    if (b == END_IF || b == END_SWITCH || b == BREAK_) return -1;
    return 0;
}

// --- text parsing ---

std::string upper(std::string s) { for (char& c : s) c = (char)std::toupper((unsigned char)c); return s; }

bool parse_int(const std::string& t, long long lo, long long hi, long long& v) {
    if (t.empty()) return false;
    errno = 0;
    char* end = nullptr;
    v = std::strtoll(t.c_str(), &end, 0);
    return errno == 0 && end && *end == 0 && v >= lo && v <= hi;
}

bool put_operands(std::vector<uint8_t>& out, const std::string& f, const std::vector<std::string>& tok, size_t first,
                  std::string& why) {
    if (tok.size() - first != f.size()) {
        why = "expects " + std::to_string(f.size()) + " operand(s), got " + std::to_string(tok.size() - first);
        return false;
    }
    for (size_t i = 0; i < f.size(); ++i) {
        const std::string& t = tok[first + i];
        long long v = 0;
        switch (f[i]) {
        case 'b':
            if (!parse_int(t, -128, 255, v)) { why = "'" + t + "' isn't a byte"; return false; }
            out.push_back((uint8_t)v);
            break;
        case 's':
            if (!parse_int(t, -128, 255, v)) { why = "'" + t + "' isn't a signed byte"; return false; }
            out.push_back((uint8_t)v);
            break;
        case 'w':
            if (!parse_int(t, 0, 0xFFFF, v)) { why = "'" + t + "' isn't 0-65535"; return false; }
            out.push_back((uint8_t)v);
            out.push_back((uint8_t)(v >> 8));
            break;
        case 'd':
            if (!parse_int(t, 0, 0xFFFFFFFFll, v)) { why = "'" + t + "' isn't a u32"; return false; }
            for (int k = 0; k < 4; ++k) out.push_back((uint8_t)(v >> (8 * k)));
            break;
        case 'f': {
            char* end = nullptr;
            const float fv = std::strtof(t.c_str(), &end);
            if (!end || *end) { why = "'" + t + "' isn't a number"; return false; }
            uint8_t b[4];
            std::memcpy(b, &fv, 4);
            out.insert(out.end(), b, b + 4);
            break;
        }
        }
    }
    return true;
}

bool hex_suffix(const std::string& name, const char* prefix, int& v) {
    const size_t n = std::strlen(prefix);
    if (name.size() != n + 2 || name.compare(0, n, prefix) != 0) return false;
    char* end = nullptr;
    v = (int)std::strtol(name.c_str() + n, &end, 16);
    return end && *end == 0;
}

int find_condition(const std::string& name) {
    for (int c = 0; c <= 0x30; ++c) if (name == kConds[c]) return c;
    int v = 0;
    if (hex_suffix(name, "C_", v) && tables().cond[v]) return v;
    return -1;
}

int find_op(const std::string& name) {
    for (const Named& n : kOps) if (name == n.name) return n.code;
    int v = 0;
    if (hex_suffix(name, "OP_", v) && v <= kMaxDx9Code && tables().code[v]) return v;
    return -1;
}

} // namespace

int code_size(uint8_t op) { return tables().code[op]; }
int condition_size(uint8_t cond) { return tables().cond[cond]; }

bool validate(const uint8_t* data, size_t size, std::string* error, size_t* length) {
    if (!data) { if (error) *error = "no script"; return false; }
    size_t i = 0;
    int depth = 0;
    while (i < size) {
        if (data[i] == END_DATA) {
            if (length) *length = i + 1;
            if (depth != 0) { if (error) *error = std::to_string(depth) + " block(s) not closed at END"; return false; }
            return true;
        }
        std::string why;
        const int n = instruction_size(data + i, size - i, &why);
        if (!n) { if (error) *error = why + " at byte " + std::to_string(i); return false; }
        depth += depth_change(data[i]);
        if (depth < 0) { if (error) *error = "END_IF / END_SWITCH without a block at byte " + std::to_string(i); return false; }
        i += n;
    }
    if (error) *error = "no END";
    return false;
}

std::string disassemble(const uint8_t* data, size_t size) {
    std::string s;
    size_t i = 0;
    int depth = 0;
    while (i < size) {
        const uint8_t b = data[i];
        if (b == END_DATA) { s += "END\n"; return s; }
        std::string why;
        const int n = instruction_size(data + i, size - i, &why);
        if (!n) { s += "# error: " + why + "\n"; return s; }
        const int d = depth_change(b);
        if (d < 0) depth = depth > 0 ? depth - 1 : 0;
        const int indent = b == ELSE_ && depth > 0 ? depth - 1 : depth;
        std::string line(2 * indent, ' ');
        if (b >= 0xF0) {
            line += kCtrl[b - 0xF0];
            if (b <= OR_NIF) {
                line += " " + cond_name(data[i + 1]);
                append_operands(line, data + i + 2, condition_fmt(data[i + 1]));
            }
        } else {
            line += op_name(b);
            append_operands(line, data + i + 1, op_fmt(b));
        }
        s += line + "\n";
        if (d > 0) ++depth;
        i += n;
    }
    s += "# error: no END\n";
    return s;
}

bool assemble(const std::string& text, std::vector<uint8_t>& out, std::string* error) {
    out.clear();
    size_t pos = 0;
    int line_no = 0;
    bool ended = false;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) nl = text.size();
        std::string line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;
        if (const size_t h = line.find('#'); h != std::string::npos) line.resize(h);
        std::vector<std::string> tok;
        std::string cur;
        for (char ch : line) {
            if (std::isspace((unsigned char)ch) || ch == ',') { if (!cur.empty()) { tok.push_back(cur); cur.clear(); } }
            else cur += ch;
        }
        if (!cur.empty()) tok.push_back(cur);
        if (tok.empty()) continue;
        auto fail = [&](const std::string& why) {
            if (error) *error = "line " + std::to_string(line_no) + ": " + why;
            return false;
        };
        if (ended) return fail("text after END");
        const std::string word = upper(tok[0]);
        std::string why;
        int ctrl = -1;
        for (int k = 0; k < 16; ++k) if (kCtrl[k] && word == kCtrl[k]) ctrl = k;
        if (ctrl >= 0) {
            const uint8_t b = (uint8_t)(0xF0 + ctrl);
            out.push_back(b);
            if (b == END_DATA) {
                if (tok.size() != 1) return fail("END takes no operands");
                ended = true;
                continue;
            }
            if (b <= OR_NIF) {
                if (tok.size() < 2) return fail(word + " needs a condition");
                const int c = find_condition(upper(tok[1]));
                if (c < 0) return fail("unknown condition '" + tok[1] + "'");
                out.push_back((uint8_t)c);
                if (!put_operands(out, condition_fmt(c), tok, 2, why)) return fail(tok[1] + " " + why);
            } else if (tok.size() != 1) {
                return fail(word + " takes no operands");
            }
            continue;
        }
        const int op = find_op(word);
        if (op < 0) return fail("unknown instruction '" + tok[0] + "'");
        out.push_back((uint8_t)op);
        const std::string f = op_fmt(op);
        if (fmt_bytes(f) + 1 != code_size((uint8_t)op)) return fail("internal: operand table of " + word);
        if (!put_operands(out, f, tok, 1, why)) return fail(word + " " + why);
    }
    if (!ended) out.push_back(END_DATA);
    std::string why;
    if (!validate(out.data(), out.size(), &why)) {
        if (error) *error = why;
        return false;
    }
    return true;
}

} // namespace event_script
