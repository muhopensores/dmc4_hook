#include "MotionRegistry.hpp"
#include "LmtMerge.hpp"
#include "Devil4.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace motion_registry {

namespace {
    // ---- game side (DX9; RE/player_move_pipeline.md "Registry RE") --------------------------------------------------
    constexpr uintptr_t kPlayerId      = 0x1494;     // uPlayer::mPlayerID: 1 Nero, 0 Dante
    constexpr uintptr_t kCoat          = 0x52C0;     // uPlayerNero / uPlayerDante::mCoat (kMdlCoat, its uModel first)
    constexpr uintptr_t kCoatDevil     = 0xCFC0;     // uPlayerDante::mCoatDevil: the coat in Devil Trigger (setupMajinModel)
    constexpr uintptr_t kDevilStand    = 0xCDF8;     // uPlayerNero::mpDevilStand (uPlNeroDevil*)
    constexpr uintptr_t kStandSub      = 0x1374;     // uPlNeroDevil: its sub-model (uModel*)
    constexpr uintptr_t kMotionCtrl    = 0x2894, kMotionCtrlSize = 0x3C;   // uPlayer::mMotionCtrl[7]
    constexpr uintptr_t kMcMotionNo    = 0x14, kMcMotionNoOld = 0x18, kMcIsInterpolate = 0x35;   // cMotionCtrl

    void set_motion_list(uintptr_t model, uintptr_t lmt, int bank) {   // uModel::setMotionList 0xA26340, __thiscall
        ((void(__fastcall*)(uintptr_t, int, uintptr_t, int))0x00A26340)(model, 0, lmt, bank);
    }

    struct MemStream {                   // MtMemoryStream (vftable 0xBFF448): what rMotionList::load reads (+0x30 read, +0x44 size)
        uintptr_t vtable;
        const void* data;
        uint32_t pos;
        uint32_t size;
        uint32_t flags;                  // 1 readable (4 would free `data` through TempAllocator)
    };

    // An rMotionList made the way sResource::create makes one, loaded from memory, never in sResource's cache: its last
    // release (uModel::setMotionList or the model's destructor) frees it like a loaded list.
    uintptr_t make_list(const std::vector<uint8_t>& bytes, int bank) {
        void* list = ((void*(__thiscall*)(void*))0x00A69E90)((void*)0x00EAE348);   // rMotionList::DTI newInstance: refcount 1
        if (!list) return 0;
        const uintptr_t l = (uintptr_t)list;
        std::snprintf((char*)(l + 0x04), 64, "motion\\registry_bank%d", bank);       // cResource path: a name for debuggers only
        *(uint64_t*)(l + 0x58) = 0;                                                  // mID: no sResource table entry
        MemStream s{ 0x00BFF448, bytes.data(), 0, (uint32_t)bytes.size(), 1 };
        void** vt = *(void***)list;
        if (!((bool(__thiscall*)(void*, void*))vt[8])(list, &s)) {                  // rMotionList::load 0xA69F20
            devil4_sdk::release_resource((cResource*)list);
            return 0;
        }
        *(uint32_t*)(l + 0x50) = (uint32_t)bytes.size();                             // mSize, mState "loaded", as create_file_load
        *(uint32_t*)(l + 0x4C) |= 1;
        return l;
    }

    int character_of(uintptr_t pl) { return *(int*)(pl + kPlayerId) == 1 ? NERO : DANTE; }

    // ---- the sets -------------------------------------------------------------------------------------------------
    constexpr int kKinds = 2;            // per target, the models a set has files for: PLAYER body / coat, STAND stand / sub

    std::string norm(std::string p) {
        for (char& ch : p) ch = ch == '/' ? '\\' : (char)std::tolower((unsigned char)ch);
        return p;
    }

    struct Set { SetDesc d; int refs = 0; };

    const std::string& file_of(const SetDesc& d, int character, int kind) {
        static const std::string none;
        if (d.target == STAND) return character != NERO ? none : kind == 0 ? d.stand : d.stand_sub;
        return kind == 0 ? d.body[character] : d.coat[character];
    }
    bool shared(const SetDesc& d) { return d.target == PLAYER && !d.body[NERO].empty() && !d.body[DANTE].empty(); }

    std::mutex g_mx;
    std::vector<Set> g_sets;
    struct Reservation { std::string owner; int character, bank; };
    std::vector<Reservation> g_reserved;
    std::set<int> g_foreign;             // character * 64 + target * 16 + bank: held another LMT when the registry built
    uint32_t g_version = 1;              // bumped by every change that can move a set

    bool blocked(int character, int target, int bank) {
        if (g_foreign.count(character * 64 + target * 16 + bank)) return true;
        if (target != PLAYER) return false;
        for (const Reservation& r : g_reserved) if (r.character == character && r.bank == bank) return true;
        return false;
    }

    std::vector<int> pool(int character, int target, bool shared_set) {
        std::vector<int> banks;
        if (target == STAND) { for (int b = 4; b <= 14; ++b) banks.push_back(b); return banks; }
        for (int b = 10; b <= 14; ++b) banks.push_back(b);
        // Nero's setup leaves 2, 4, 6-9 empty too, but not 2: in DT the stand plays the motion word Nero's Red Queen
        // requests carry (setRedQueenMotion -> mYamatoParam) from its own banks, and its bank 2 is pl020_03
        if (character == NERO && !shared_set) for (int b : { 4, 6, 7, 8, 9 }) banks.push_back(b);
        return banks;
    }

    // ---- a layout: where every set sits for one (character, target), and the merged bytes per bank and model kind ---
    struct Placement { Handle h; int bank, first, count; };
    struct BankImage { int bank = 0; bool has[kKinds] = {}; std::vector<uint8_t> bytes[kKinds]; };
    struct Layout {
        uint32_t version = 0;
        std::vector<Placement> places;
        std::vector<BankImage> banks;
        std::map<Handle, std::string> unplaced;     // set -> why it has no blocks
        const Placement* find(Handle h) const {
            for (const Placement& p : places) if (p.h == h) return &p;
            return nullptr;
        }
        bool uses(int bank) const {
            for (const BankImage& b : banks) if (b.bank == bank) return true;
            return false;
        }
    };
    std::shared_ptr<Layout> g_layouts[2][2];        // [character][target]

    bool read_file(const std::string& path, std::vector<uint8_t>& out) {
        std::ifstream f(".\\nativePC\\" + path + ".lmt", std::ios::binary);
        if (!f) return false;
        out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        return true;
    }

    const char* kCharName[2] = { "Nero", "Dante" };

    std::shared_ptr<Layout> build_layout(int c, int t) {
        auto L = std::make_shared<Layout>();
        L->version = g_version;
        std::map<std::string, std::vector<uint8_t>> files;          // read once per build
        std::map<std::string, int> counts;
        auto load = [&](const std::string& path, std::string& why) -> const std::vector<uint8_t>* {
            const std::string key = norm(path);
            auto it = files.find(key);
            if (it == files.end()) {
                std::vector<uint8_t> b;
                if (!read_file(path, b)) { why = "nativePC\\" + path + ".lmt not found"; counts[key] = -1; files[key]; return nullptr; }
                std::string bad;
                counts[key] = lmt_merge::block_count(b, &bad);
                if (counts[key] < 0) { why = "nativePC\\" + path + ".lmt: " + bad; files[key]; return nullptr; }
                it = files.emplace(key, std::move(b)).first;
            }
            if (counts[key] < 0) { why = "nativePC\\" + path + ".lmt is missing or not a v49 LMT"; return nullptr; }
            return &it->second;
        };
        struct Cand { Handle h; bool shared; std::string key; const std::vector<uint8_t>* file[kKinds]; int count; };
        std::vector<Cand> cands;
        for (Handle h = 0; h < (Handle)g_sets.size(); ++h) {
            const Set& s = g_sets[h];
            if (s.refs <= 0 || s.d.target != t || file_of(s.d, c, 0).empty()) continue;
            Cand cd{ h, shared(s.d), norm(file_of(s.d, c, 0)), { nullptr, nullptr }, 0 };
            std::string why;
            cd.file[0] = load(file_of(s.d, c, 0), why);
            if (!cd.file[0]) { L->unplaced[h] = why; spdlog::warn("motion_registry: {} ({}): {}", s.d.owner, kCharName[c], why); continue; }
            cd.count = lmt_merge::block_count(*cd.file[0]);
            if (cd.count > lmt_merge::kMaxBlocks) { L->unplaced[h] = "more than 256 blocks"; spdlog::warn("motion_registry: {}: more than 256 blocks", s.d.owner); continue; }
            for (int k = 1; k < kKinds; ++k) {
                const std::string& p = file_of(s.d, c, k);
                if (p.empty()) continue;
                std::string w;
                cd.file[k] = load(p, w);
                if (!cd.file[k]) spdlog::warn("motion_registry: {} ({}): {}: that model keeps its pose on these motions", s.d.owner, kCharName[c], w);
                else if (lmt_merge::block_count(*cd.file[k]) > cd.count)
                    spdlog::warn("motion_registry: {} ({}): {} has more blocks than the body file; the extra ones are left out", s.d.owner, kCharName[c], p);
            }
            cands.push_back(cd);
        }
        std::stable_sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) {
            if (a.shared != b.shared) return a.shared;
            if (a.key != b.key) return a.key < b.key;
            return a.h < b.h;
        });
        std::map<int, int> used;                                    // bank -> blocks taken
        std::map<int, std::vector<const Cand*>> by_bank;
        for (const Cand& cd : cands) {
            bool placed = false;
            for (int b : pool(c, t, cd.shared)) {
                if (blocked(c, t, b) || used[b] + cd.count > lmt_merge::kMaxBlocks) continue;
                L->places.push_back({ cd.h, b, used[b], cd.count });
                by_bank[b].push_back(&cd);
                used[b] += cd.count;
                placed = true;
                break;
            }
            if (!placed) {
                L->unplaced[cd.h] = "no free bank has room";
                spdlog::warn("motion_registry: {} ({}): no free bank has room for its {} blocks", g_sets[cd.h].d.owner, kCharName[c], cd.count);
            }
        }
        for (auto& [bank, list] : by_bank) {
            BankImage img;
            img.bank = bank;
            for (int k = 0; k < kKinds; ++k) {
                std::vector<lmt_merge::Piece> pieces;
                for (const Cand* cd : list) {
                    const Placement* p = L->find(cd->h);
                    pieces.push_back({ cd->file[k], p->first, p->count });
                    if (cd->file[k]) img.has[k] = true;
                }
                if (!img.has[k]) continue;
                std::string why;
                if (!lmt_merge::merge(pieces, used[bank], img.bytes[k], &why)) {
                    spdlog::error("motion_registry: {} bank {}: merge failed: {}", kCharName[c], bank, why);
                    img.has[k] = false;
                }
            }
            L->banks.push_back(std::move(img));
        }
        spdlog::info("motion_registry: {} {} layout v{}: {} set(s) in {} bank(s), {} not placed", kCharName[c],
                     t == PLAYER ? "player" : "stand", L->version, L->places.size(), L->banks.size(), L->unplaced.size());
        return L;
    }

    std::shared_ptr<Layout> layout_for(int c, int t) {
        std::shared_ptr<Layout>& L = g_layouts[c][t];
        if (!L || L->version != g_version) L = build_layout(c, t);
        return L;
    }

    // ---- per player object: what the registry put on its models ----------------------------------------------------
    struct Installed { uintptr_t model; int bank; uintptr_t list; };
    struct PlayerBanks {
        uintptr_t player = 0;
        uint32_t last = 0;
        std::shared_ptr<Layout> layout[2];          // per target: the layout the models hold
        std::vector<Installed> lists[2];
    };
    PlayerBanks g_players[8];
    uint32_t g_tick = 0;

    PlayerBanks& state_for(uintptr_t pl) {
        ++g_tick;
        for (PlayerBanks& p : g_players) if (p.player == pl) { p.last = g_tick; return p; }
        PlayerBanks* victim = &g_players[0];
        for (PlayerBanks& p : g_players) {
            if (!p.player) { victim = &p; break; }
            if (p.last < victim->last) victim = &p;
        }
        *victim = PlayerBanks{};
        victim->player = pl;
        victim->last = g_tick;
        return *victim;
    }

    // (kind, model) pairs of a target on this player; the stand's are absent until the stand exists
    std::vector<std::pair<int, uintptr_t>> models_of(uintptr_t pl, int c, int t) {
        std::vector<std::pair<int, uintptr_t>> m;
        if (t == PLAYER) {
            m.push_back({ 0, pl });
            m.push_back({ 1, pl + kCoat });
            if (c == DANTE) m.push_back({ 1, pl + kCoatDevil });
        } else if (c == NERO) {
            const uintptr_t stand = *(uintptr_t*)(pl + kDevilStand);
            if (stand) {
                m.push_back({ 0, stand });
                if (const uintptr_t sub = *(uintptr_t*)(stand + kStandSub)) m.push_back({ 1, sub });
            }
        }
        return m;
    }

    bool intact(const PlayerBanks& ps, int t) {     // every list we installed is still where we put it
        if (!ps.layout[t]) return false;
        for (const Installed& i : ps.lists[t])
            if (devil4_sdk::get_motion_bank((void*)i.model, i.bank) != i.list) return false;
        return true;
    }

    // A body motion controller plays one of L's banks, or still blends out of one (the old motion while mIsInterpolate).
    bool playing_from(uintptr_t pl, const Layout& L) {
        for (int i = 0; i < 3; ++i) {
            const uintptr_t c = pl + kMotionCtrl + kMotionCtrlSize * i;
            if (L.uses((*(uint32_t*)(c + kMcMotionNo) >> 8) & 0xF)) return true;
            if (*(bool*)(c + kMcIsInterpolate) && L.uses((*(uint32_t*)(c + kMcMotionNoOld) >> 8) & 0xF)) return true;
        }
        return false;
    }

    // Put the current layout on the player's models when it isn't there yet (or was lost with a new player object).
    void ensure(PlayerBanks& ps, uintptr_t pl, int c, int t) {
        std::shared_ptr<Layout> want = layout_for(c, t);
        const bool ok = intact(ps, t);
        if (ok && ps.layout[t] == want) return;
        if (ok && t == PLAYER && playing_from(pl, *ps.layout[t])) return;         // swap once no registry motion plays
        const auto models = models_of(pl, c, t);
        if (models.empty()) return;
        if (!ok) ps.lists[t].clear();                                               // a new object at this address: nothing of ours is on it
        // a bank of the new layout that holds a list nobody here put there belongs to another mod: never overwrite it
        for (const BankImage& img : want->banks) {
            const uintptr_t cur = devil4_sdk::get_motion_bank((void*)models[0].second, img.bank);
            if (!cur) continue;
            bool ours = false;
            for (const Installed& i : ps.lists[t]) if (i.model == models[0].second && i.bank == img.bank && i.list == cur) ours = true;
            if (ours) continue;
            spdlog::warn("motion_registry: {} bank {} already holds an LMT the registry didn't put there (a mod that loads its own "
                         "bank without motion_registry::reserve?); the registry leaves that bank alone", kCharName[c], img.bank);
            g_foreign.insert(c * 64 + t * 16 + img.bank);
            ++g_version;
            return;                                                                 // the next call places the sets elsewhere
        }
        for (const Installed& i : ps.lists[t])                                     // take the old layout's lists off
            if (devil4_sdk::get_motion_bank((void*)i.model, i.bank) == i.list) set_motion_list(i.model, 0, i.bank);
        ps.lists[t].clear();
        for (const BankImage& img : want->banks)
            for (const auto& [kind, model] : models) {
                if (!img.has[kind]) continue;
                const uintptr_t list = make_list(img.bytes[kind], img.bank);
                if (!list) { spdlog::error("motion_registry: {} bank {}: the merged LMT did not load", kCharName[c], img.bank); continue; }
                set_motion_list(model, list, img.bank);                             // the model's reference
                devil4_sdk::release_resource((cResource*)list);                     // ours
                ps.lists[t].push_back({ model, img.bank, list });
            }
        ps.layout[t] = want;
        spdlog::info("motion_registry: layout v{} on {} {:#x} ({} list(s))", want->version, kCharName[c], pl, ps.lists[t].size());
    }

    const Set* live(Handle h) { return h >= 0 && h < (Handle)g_sets.size() && g_sets[h].refs > 0 ? &g_sets[h] : nullptr; }

    bool same(const SetDesc& a, const SetDesc& b) {
        if (a.target != b.target) return false;
        for (int c = 0; c < 2; ++c) if (norm(a.body[c]) != norm(b.body[c]) || norm(a.coat[c]) != norm(b.coat[c])) return false;
        return norm(a.stand) == norm(b.stand) && norm(a.stand_sub) == norm(b.stand_sub);
    }
}

Handle add(const SetDesc& d) {
    std::lock_guard<std::mutex> lock(g_mx);
    for (Handle h = 0; h < (Handle)g_sets.size(); ++h)
        if (g_sets[h].refs > 0 && same(g_sets[h].d, d)) { ++g_sets[h].refs; return h; }
    g_sets.push_back(Set{ d, 1 });
    ++g_version;
    return (Handle)g_sets.size() - 1;
}

void remove(Handle h) {
    std::lock_guard<std::mutex> lock(g_mx);
    if (h < 0 || h >= (Handle)g_sets.size() || g_sets[h].refs <= 0) return;
    if (--g_sets[h].refs == 0) ++g_version;
}

void reserve(const char* owner, int character, int bank) {
    std::lock_guard<std::mutex> lock(g_mx);
    for (Reservation& r : g_reserved)
        if (r.owner == owner && r.character == character) {
            if (r.bank == bank) return;                     // unchanged: no rebuild
            if (bank >= 0) r.bank = bank;
            else g_reserved.erase(g_reserved.begin() + (&r - g_reserved.data()));
            ++g_version;
            return;
        }
    if (bank < 0) return;
    g_reserved.push_back({ owner, character, bank });
    ++g_version;
}

bool ready(Handle h, uintptr_t player) {
    std::lock_guard<std::mutex> lock(g_mx);
    const Set* s = live(h);
    if (!s || !player) return false;
    const int c = character_of(player), t = s->d.target;
    if (file_of(s->d, c, 0).empty()) return false;
    PlayerBanks& ps = state_for(player);
    for (int i = 0; i < 4; ++i) {                                   // a bank found taken moves the sets: place them again
        const uint32_t v = g_version;
        ensure(ps, player, c, t);
        if (g_version == v) break;
    }
    return ps.layout[t] && ps.layout[t]->find(h) && intact(ps, t);
}

int word(Handle h, uintptr_t player, int block) {
    std::lock_guard<std::mutex> lock(g_mx);
    const Set* s = live(h);
    if (!s || !player || block < 0) return -1;
    const int t = s->d.target;
    for (const PlayerBanks& ps : g_players) {
        if (ps.player != player || !ps.layout[t]) continue;
        const Placement* p = ps.layout[t]->find(h);
        if (!p || block >= p->count) return -1;
        return (p->bank << 8) | (p->first + block);
    }
    return -1;
}

bool placed(Handle h, uintptr_t player) {
    std::lock_guard<std::mutex> lock(g_mx);
    const Set* s = live(h);
    if (!s || !player) return false;
    const int t = s->d.target;
    for (const PlayerBanks& ps : g_players)
        if (ps.player == player) return ps.layout[t] && ps.layout[t]->find(h) && intact(ps, t);
    return false;
}

void refresh() {
    std::lock_guard<std::mutex> lock(g_mx);
    ++g_version;
}

std::string describe(Handle h, uintptr_t player) {
    std::lock_guard<std::mutex> lock(g_mx);
    const Set* s = live(h);
    if (!s) return "not registered";
    const int t = s->d.target;
    if (player) {
        for (const PlayerBanks& ps : g_players) {
            if (ps.player != player || !ps.layout[t]) continue;
            if (const Placement* p = ps.layout[t]->find(h))
                return "bank " + std::to_string(p->bank) + " blocks " + std::to_string(p->first) + "-" + std::to_string(p->first + p->count - 1);
            auto it = ps.layout[t]->unplaced.find(h);
            return it != ps.layout[t]->unplaced.end() ? it->second : "not on this player yet (a rebuild waits for the current motion)";
        }
    }
    return "not built for this player yet";
}

} // namespace motion_registry
