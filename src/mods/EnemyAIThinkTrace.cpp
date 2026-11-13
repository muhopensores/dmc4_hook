#include "EnemyAIThinkTrace.hpp"
#include "../mod.hpp"
#include "../sdk/MtDTI.hpp"
#include "../sdk/Players.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>

namespace think_trace {

bool enabled = false;

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// ---- Table names ---------------------------------------------------------------------------------------------------
struct TableName { uint32_t addr; uint16_t rows; const char* name; };
const TableName kTables[] = {
#include "EnemyAIThinkTables.inc"
};
constexpr size_t kTableCount = sizeof(kTables) / sizeof(kTables[0]);
constexpr uintptr_t kRowSize = 0x68;

// The named table holding `row`, and the row's index in it; null when it's in none.
const TableName* table_of(uintptr_t row, int* index) {
    const TableName* end = kTables + kTableCount;
    const TableName* t = std::upper_bound(kTables, end, row, [](uintptr_t r, const TableName& e) { return r < e.addr; });
    if (t == kTables) return nullptr;
    --t;
    if (row >= t->addr + t->rows * kRowSize) return nullptr;
    if (index) *index = (int)((row - t->addr) / kRowSize);
    return t;
}

// ---- Game layout (DX9) ---------------------------------------------------------------------------------------------
// cThinkMgr
constexpr uintptr_t kCheck      = 0x04;    // mbCheck[21]
constexpr uintptr_t kLeng       = 0x4C, kDegAng = 0x50, kHeight = 0x54, kMotState = 0x58, kLv = 0x5C;
constexpr uintptr_t kRndStart   = 0x60, kRndProb = 0x64, kRndSubProb = 0x68;
constexpr uintptr_t kTable      = 0x6C;    // mpThinkTbl (the table being walked)
constexpr uintptr_t kTimers     = 0xFC;    // mkTimer[32] {no, group, timer}
constexpr uintptr_t kCounts     = 0x27C;   // mkPassCount[32] {no, group, count}
constexpr uintptr_t kNextFlg    = 0x3FC;
constexpr uintptr_t kActPassNo  = 0x428;
constexpr uintptr_t kBitCtrl    = 0x42C;
// kTHINKDATA
constexpr uintptr_t rFlagLo = 0x00, rFlagHi = 0x04, rLengA = 0x08, rLengB = 0x0C, rAngA = 0x10, rAngB = 0x14;
constexpr uintptr_t rHeightA = 0x18, rHeightB = 0x1C, rAction = 0x28, rTable = 0x2C, rBit = 0x38, rSys = 0x40;
constexpr float kUnbounded = 1.0e30f;
// uPlayer lock-on
constexpr uintptr_t kLocked = 0x3078, kLockTarget = 0x3080;

// ---- Records -------------------------------------------------------------------------------------------------------
enum Reason : uint8_t {
    R_NONE, R_CHAIN_ONLY, R_ROLL, R_REPEAT, R_MOTION, R_LEVEL, R_TOO_CLOSE, R_TOO_FAR, R_ANGLE, R_HEIGHT, R_COOLDOWN,
    R_COUNT, R_NEEDS_FLAG, R_FLAG_SET, R_BITS, R_CLASS_CHECK
};
const char* kReasonText[] = {
    "", "only while continuing a chain", "lost the random roll", "same move as last time", "motion state",
    "level", "too close", "too far", "angle", "height", "cooldown running", "pick count running", "needs flag %d",
    "flag %d forbids it", "bit field", "class check / callback"
};

struct Step {
    uintptr_t row;      // in its table (0 = not found)
    int32_t action;
    int32_t result;     // thinkCheck: -1 no, -2 go to its table, -3 skip, -4 go to its table (chain cleared), else the move
    uint8_t reason;
    int8_t arg;
};
constexpr int kMaxSteps = 96;
struct Decision {
    uint32_t serial = 0;
    uintptr_t enemy = 0, mgr = 0, start = 0;
    uint32_t mot = 0;
    float leng = 0, deg = 0, height = 0;
    int lv = 0;
    uint8_t check[21] = {};
    uint32_t bits = 0;
    int result = -1;
    int n = 0;
    bool truncated = false;
    Step steps[kMaxSteps];
};
constexpr int kKeep = 12;
Decision g_hist[kKeep];
int g_head = 0, g_count = 0;     // next slot, filled slots
uint32_t g_serial = 0;
std::mutex g_mx;

Decision g_cur;
bool g_recording = false;
bool g_paused = false;
uintptr_t g_enemy = 0, g_enemy_size = 0;

uintptr_t dti_of(uintptr_t unit) {
    return (uintptr_t)((MtDTI*(__fastcall*)(uintptr_t, int))(at<uintptr_t>(at<uintptr_t>(unit, 0), 0x10)))(unit, 0);
}

// Player 1's lock-on target becomes the traced unit (any unit but a player: enemies and the boss Dante).
void pick_target() {
    const uintptr_t pl = player_info::player(0);
    if (!pl || !at<uint8_t>(pl, kLocked)) return;
    const uintptr_t t = at<uintptr_t>(pl, kLockTarget);
    const uintptr_t model = t ? at<uintptr_t>(t, 0x08) : 0;
    if (!model || model == g_enemy || player_info::is_player(model) || !at<uintptr_t>(model, 0)) return;
    g_enemy = model;
    g_enemy_size = at<uintptr_t>(dti_of(model), 0x18);   // MtDTI::m_size
}

bool owned(uintptr_t mgr) {
    return g_enemy && mgr >= g_enemy && mgr < g_enemy + g_enemy_size;
}

// The checkTimer / checkPassCount test: an entry for this action (or group) still running.
bool running(uintptr_t mgr, uintptr_t list, float action, float group) {
    for (int i = 0; i < 32; ++i) {
        const uintptr_t e = mgr + list + 12 * i;
        const bool match = (group == -1.0f || group == 0.0f) ? (action != -1.0f && at<float>(e, 0) == action)
                                                              : at<float>(e, 4) == group;
        if (match && at<float>(e, 8) >= 0.0f) return true;
    }
    return false;
}

int16_t wrap_deg(float x) {
    const uint16_t u = (uint16_t)(int)((x + 360.0f) * 65536.0f * 0.0027777778f + 0.5f);
    return (int16_t)(int)((float)u * 360.0f * 0.000015258789f);
}

// The first gate of thinkCheck the row failed (it returned -1), in thinkCheck's order.
Reason why(uintptr_t mgr, uintptr_t row, const Pre& pre, int8_t& arg) {
    const uint32_t lo = at<uint32_t>(row, rFlagLo), hi = at<uint32_t>(row, rFlagHi);
    const int action = at<int>(row, rAction);
    const float* sys = &at<float>(row, rSys);
    if ((int32_t)lo < 0 && !pre.next) return R_CHAIN_ONLY;
    if (pre.rnd_start || (lo & 0x400000)) {
        const float prob = at<float>(mgr, kRndProb), sub = pre.rnd_start ? pre.sub : 0.0f;
        if (prob < sub || sub + sys[4] <= prob) return R_ROLL;
    }
    if (hi & 0x80000000u) {
        if (action == -1 || action == pre.act_pass) return R_REPEAT;
    }
    if ((lo & 2) && !(at<uint32_t>(mgr, kMotState) & 5)) return R_MOTION;
    if (lo & 0x10) {
        const float lv = (float)at<int>(mgr, kLv);
        bool ok = false;
        if (lo & 0x20) ok = lv > sys[3];
        else if (lo & 0x40) ok = sys[3] > lv;
        if (!ok && !((lo & 0x80) && lv == sys[3])) return R_LEVEL;
    }
    const float leng = at<float>(mgr, kLeng);
    if (at<float>(row, rLengA) < kUnbounded && at<float>(row, rLengA) > leng) return R_TOO_CLOSE;
    if (at<float>(row, rLengB) < kUnbounded && leng > at<float>(row, rLengB)) return R_TOO_FAR;
    const float aa = at<float>(row, rAngA), ab = at<float>(row, rAngB);
    if (aa < kUnbounded && ab < kUnbounded) {
        int16_t d, a, b;
        if (aa <= ab) {
            d = (int16_t)(int)at<float>(mgr, kDegAng);
            a = (int16_t)(int)aa;
            b = (int16_t)(int)ab;
        } else {
            d = wrap_deg(at<float>(mgr, kDegAng));
            a = wrap_deg(aa);
            b = wrap_deg(ab);
        }
        if (a <= b ? (a > d || d > b) : (a >= d && d >= b)) return R_ANGLE;
    }
    const float ha = at<float>(row, rHeightA), hb = at<float>(row, rHeightB), h = at<float>(mgr, kHeight);
    if (ha < kUnbounded) {
        if (hb < kUnbounded && ha > hb) {
            if (h <= ha && hb <= h) return R_HEIGHT;
        } else if (ha > h) {
            return R_HEIGHT;
        }
    }
    if (!(ha < kUnbounded && hb < kUnbounded && ha > hb) && hb < kUnbounded && h > hb) return R_HEIGHT;
    if (!(hi & 0x20000000)) {
        const float group = (lo & 0x100) ? sys[2] : -1.0f;
        if ((lo & 0x400) && running(mgr, kTimers, (float)action, group)) return R_COOLDOWN;
        if (!(lo & 0x400) && (lo & 0x200) && running(mgr, kCounts, (float)action, group)) return R_COUNT;
    }
    const uint8_t* check = &at<uint8_t>(mgr, kCheck);
    for (int k = 0; k < 7; ++k) {
        if ((lo & (0x8000u << k)) && !check[k]) { arg = (int8_t)k; return R_NEEDS_FLAG; }
        if ((lo & (0x800000u << k)) && check[k]) { arg = (int8_t)k; return R_FLAG_SET; }
    }
    for (int k = 0; k < 14; ++k) {
        if ((hi & (1u << (2 * k))) && !check[7 + k]) { arg = (int8_t)(7 + k); return R_NEEDS_FLAG; }
        if ((hi & (2u << (2 * k))) && check[7 + k]) { arg = (int8_t)(7 + k); return R_FLAG_SET; }
    }
    const uint32_t bits_hi = at<uint32_t>(row, rBit + 4), ctrl = at<uint32_t>(mgr, kBitCtrl);
    const uint32_t need = bits_hi & 0xFFFF, forbid = bits_hi >> 16;
    if ((need && (need & ctrl) != need) || (forbid && (forbid & ctrl))) return R_BITS;
    return R_CLASS_CHECK;
}

void label_row(char* out, size_t size, uintptr_t row) {
    int index = 0;
    if (const TableName* t = row ? table_of(row, &index) : nullptr) {
        snprintf(out, size, "%s[%d]", t->name, index);
    } else if (row) {
        snprintf(out, size, "%08X", (uint32_t)row);
    } else {
        snprintf(out, size, "?");
    }
}

void label_table(char* out, size_t size, uintptr_t table) {
    int index = 0;
    const TableName* t = table ? table_of(table, &index) : nullptr;
    if (t && index == 0) snprintf(out, size, "%s", t->name);
    else label_row(out, size, table);
}

} // namespace

bool begin(uintptr_t mgr, uint32_t mot, float leng, float deg, float height, int lv) {
    if (!enabled || g_paused) return false;
    pick_target();
    if (!owned(mgr)) return false;
    Decision& d = g_cur;
    d.serial = ++g_serial;
    d.enemy = g_enemy;
    d.mgr = mgr;
    d.start = at<uintptr_t>(mgr, kTable);
    d.mot = mot;
    d.leng = leng;
    d.deg = deg;
    d.height = height;
    d.lv = lv;
    std::memcpy(d.check, &at<uint8_t>(mgr, kCheck), sizeof d.check);
    d.bits = at<uint32_t>(mgr, kBitCtrl);
    d.result = -1;
    d.n = 0;
    d.truncated = false;
    g_recording = true;
    return true;
}

bool recording(uintptr_t mgr) {
    return g_recording && mgr == g_cur.mgr;
}

Pre before(uintptr_t mgr, const void* row) {
    Pre p;
    p.next = at<uint8_t>(mgr, kNextFlg);
    p.rnd_start = at<uint8_t>(mgr, kRndStart);
    p.sub = at<float>(mgr, kRndSubProb);
    p.act_pass = at<int>(mgr, kActPassNo);
    // the row's place: GetActionNo copies mpThinkTbl[i] onto the stack, so find the copy in the table being walked
    const uintptr_t table = at<uintptr_t>(mgr, kTable);
    int limit = 64;
    if (const TableName* t = table ? table_of(table, nullptr) : nullptr)
        limit = (int)((t->addr + t->rows * kRowSize - table) / kRowSize);
    for (int i = 0; table && i < limit; ++i) {
        if (std::memcmp((const void*)(table + i * kRowSize), row, kRowSize) == 0) {
            p.row = table + i * kRowSize;
            break;
        }
    }
    return p;
}

void after(uintptr_t mgr, const void* row, const Pre& pre, int result) {
    Decision& d = g_cur;
    if (d.n >= kMaxSteps) {
        d.truncated = true;
        return;
    }
    Step& s = d.steps[d.n++];
    s.row = pre.row;
    s.action = at<int>((uintptr_t)row, rAction);
    s.result = result;
    s.arg = 0;
    s.reason = result == -1 ? (uint8_t)why(mgr, (uintptr_t)row, pre, s.arg) : (uint8_t)R_NONE;
}

void end(int result) {
    if (!g_recording) return;
    g_recording = false;
    g_cur.result = result;
    std::lock_guard<std::mutex> lock(g_mx);
    g_hist[g_head] = g_cur;
    g_head = (g_head + 1) % kKeep;
    g_count = std::min(g_count + 1, kKeep);
}

void draw_toggle() {
    ImGui::Checkbox(_("Display Think Trace"), &enabled);
    ImGui::SameLine();
    Mod::help_marker(_("Opens a window showing how the enemy you are locked on to (player 1) chooses its moves: each "
                       "decision's inputs, every row of its AI tables it looked at, and why each row it passed over was "
                       "turned down. Lock on to an enemy to trace it; it stays traced after you let go."));
}

void draw_window() {
    if (!enabled) return;
    ImGui::SetNextWindowSize(ImVec2(720.0f, 480.0f), ImGuiCond_Once);
    if (!ImGui::Begin(_("Think Trace"), &enabled)) {
        ImGui::End();
        return;
    }
    ImGui::PushID("ThinkTrace");
    ImGui::Checkbox(_("Pause"), &g_paused);
    ImGui::SameLine();
    Mod::help_marker(_("Stop recording, to read the decisions shown."));
    ImGui::Text(_("Traced: %08X (%u table names known)"), (uint32_t)g_enemy, (unsigned)kTableCount);

    static Decision shown[kKeep];
    int count = 0, head = 0;
    {
        std::lock_guard<std::mutex> lock(g_mx);
        count = g_count;
        head = g_head;
        for (int i = 0; i < count; ++i) shown[i] = g_hist[(head - 1 - i + kKeep) % kKeep];   // newest first
    }
    char a[96], b[96];
    for (int i = 0; i < count; ++i) {
        const Decision& d = shown[i];
        label_table(a, sizeof a, d.start);
        b[0] = 0;
        for (int k = d.n - 1; k >= 0; --k)
            if (d.steps[k].result >= 0) {
                label_row(b, sizeof b, d.steps[k].row);
                break;
            }
        ImGui::PushID((int)d.serial);
        const bool open = d.result >= 0
            ? ImGui::TreeNode("decision", _("#%u from %s: move %d (%s)"), d.serial, a, d.result, b)
            : ImGui::TreeNode("decision", _("#%u from %s: nothing"), d.serial, a);
        if (open) {
            ImGui::Text(_("distance %.0f, angle %.0f, height %.0f, motion state %X, level %d, bits %X"), d.leng, d.deg,
                        d.height, d.mot, d.lv, d.bits);
            char flags[64] = {};
            int f = 0;
            for (int k = 0; k < 21; ++k)
                if (d.check[k]) f += snprintf(flags + f, sizeof flags - f, "%d ", k);
            ImGui::Text(_("flags set: %s"), f ? flags : _("none"));
            for (int k = 0; k < d.n; ++k) {
                const Step& s = d.steps[k];
                label_row(b, sizeof b, s.row);
                if (s.result >= 0) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), _("%s: move %d (picked)"), b, s.result);
                } else if (s.result == -2 || s.result == -4) {
                    label_table(a, sizeof a, s.row ? at<uintptr_t>(s.row, rTable) : 0);
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), _("%s: go to %s%s"), b, a,
                                       s.result == -4 ? _(" (chain cleared)") : "");
                } else if (s.result == -3) {
                    ImGui::Text(_("%s: passed, no move (continues)"), b);
                } else {
                    char why_text[64];
                    snprintf(why_text, sizeof why_text, kReasonText[s.reason], s.arg);
                    ImGui::TextDisabled(_("%s: move %d, no: %s"), b, s.action, why_text);
                }
            }
            if (d.truncated) ImGui::TextDisabled(_("(more rows not recorded)"));
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    ImGui::End();
}

} // namespace think_trace
