#include "CoopOnline.hpp"
#include "sdk/sDevil4Main.hpp"
#include "sdk/sCollision.hpp"
#include "sdk/sMediator.hpp"
#include "sdk/sUnit.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/Pad.hpp"
#include "sdk/MtRandom.hpp"

#include "dev/netplay/FramePacer.hpp"

#include "zpp_bits.h"
#include "dev/netplay/BattleSnapshot.hpp"
#include "dev/netplay/HeapSnapshot.hpp"
#include "sdk/FrameHooks.hpp"
#include "sdk/LocalWorld.hpp"
#include "sdk/Rollback.hpp"
#include "sdk/Players.hpp"
#include "sdk/sArea.hpp"
#include "sdk/Devil4.hpp"
#include "dev/netplay/AllocatorThings.hpp"
#include "GuiFunctions.hpp"
#include "gekkonet.h"
#include "utility/CommandLineArgs.hpp"
#include "dev/netplay/NetplayTestHarness.hpp"
#include "dev/coop/Coop.hpp"
#include "dev/coop/CoopPads.hpp"
#include "Mods.hpp"
#include <map>
#include <string>
#include <unordered_set>

static constexpr auto MAX_PLAYERS = 4;

// GekkoLib clamps the prediction window to DEFAULT_BUFF_SIZE / 2 (64) when a game session
// starts (game_session.cpp:36) but leaves the local delay alone. Both ceilings are set lower
// than GekkoLib would allow on purpose: one rollback state is ~4 MiB (BattleSnapshot.hpp) and
// the session stores prediction window + 2 of them (storage.cpp:11), so GekkoLib's own 64-frame
// ceiling would commit ~270 MiB up front in this 32-bit process; and delay + window would fill
// the 128-frame input ring (DEFAULT_BUFF_SIZE) exactly, leaving nothing for the live frame.
static constexpr uint8_t kMaxLocalDelay = 32;
static constexpr uint8_t kMaxPredictionWindow = 16;

static void fatal_msgbox(const char* text) {
    MessageBoxA(NULL, text, "DMC4", MB_ICONERROR);
    std::abort();
}

namespace spdlog {
    namespace {
    struct ScreenLog {
        std::string logstring;
        ImU32 color;
    };
    std::vector<ScreenLog> g_strings;
    std::mutex g_mutex;
    constexpr ImU32 kQuietColor = 0xFFF6FF00;
    constexpr ImU32 kDefaultColor = 0xFFEC4EFF;
    }

    void screen(std::string&& str, ImU32 col = kDefaultColor) {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (col != kQuietColor) {
            netplay_test::log(str);
        }
        g_strings.emplace_back(ScreenLog{str, col});
    }

    void hehe_draw() {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 pos = ImVec2(16.0f, 16.0f);
        std::lock_guard<std::mutex> lock(g_mutex);
        for (const auto& string : g_strings) {
            dl->AddText(g_framework->m_imfont_main, 28.0f, pos, string.color, string.logstring.c_str());
            pos.y += 32.0f;
#ifdef _DEBUG
            if(string.color != kQuietColor) {
                printf("screen: %s\n", string.logstring.c_str());
            }
#endif // _DEBUG

        }
    }

    void hehe_flush() {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_strings.clear();
    }
}

#pragma region simulation_state

// Simulation state carried across rollback frames (see the tick below).
struct SimCtx {
    uint64_t g_simTimeUs;         // part of the rollback snapshot
    bool     g_resimulating;      // current Advance is historical, not the final forward Advance
    bool     g_replaying_window;  // a load succeeded; every Advance needs deterministic timing

    // Online pacing (OnlineSession.cpp handle_frame_time): <= 1; stretched while this session
    // runs ahead of its slowest peer, so the faster machine lets the slower one catch up.
    float g_pace_scale = 1.0f;
};

// The one global MtRandom is shared by the simulated tick and by presentation code (sound,
// effects, render) that runs once per presented frame and is not part of the replay. Those draws
// do not affect the units immediately, but they shift mti, so the next tick on a re-run consumes
// a different part of the stream and AI diverges a few frames later. Keep the post-tick RNG and
// put it back at the start of every tick: only the simulated tick is allowed to advance the stream.
static netplay_snapshot::RngState g_tick_rng;
static bool g_tick_rng_pinned = false;
// no run_simulation_tick has consumed the stream since the pin was taken: the session's first
// save then records the pinned stream (the stage seed), not presentation's draws
static bool g_tick_rng_fresh = false;

static void pin_tick_rng_from_live()
{
    const MtRandom* rng = mt_random_global();
    std::memcpy(g_tick_rng.mt, rng->mt, sizeof g_tick_rng.mt);
    g_tick_rng.mti = rng->mti;
    g_tick_rng_pinned = true;
    g_tick_rng_fresh = true;
}

static void restore_tick_rng()
{
    if (!g_tick_rng_pinned) {
        return;
    }
    MtRandom* rng = mt_random_global();
    std::memcpy(rng->mt, g_tick_rng.mt, sizeof g_tick_rng.mt);
    rng->mti = g_tick_rng.mti;
}

// The pin only means something while the session that took it is ticking: a plain tick after a
// Disconnect (or with no session at all) must run on the live stream, or every tick would
// rewind to the same saved state and the game's random draws would repeat.
static void clear_tick_rng_pin()
{
    g_tick_rng_pinned = false;
    g_tick_rng_fresh = false;
}

// Set while the presented frame ran a forward simulation tick. The detour jumps into the native
// tail only then: tail(N) belongs to tick(N) one-to-one, and a presented frame that stalls on
// peer input - or waits for the connect handshake before the first advance - must not advance
// the tail state, or two machines whose presented-frame counts drift apart diverge (see the
// note above run_tail_simulation). Zero-tick frames run only the presentation systems there.
static bool g_native_tail_due = false;
static bool g_tick_probe_enabled = false;  // debug: run two ticks from one state (perturbs the run)

static void note_saved_state(int frame, uint32_t checksum, const unsigned char* bytes, uint32_t len);
static void probe_tick_determinism(sDevil4Main* main, const netplay_snapshot::BattleSnapshot& state, SimCtx& ctx);
static void verify_restore_roundtrip(sDevil4Main* main, const unsigned char* loaded, uint32_t loaded_len, uint64_t sim_time);
static void reset_desync_diff();

// Rollback restores the game's heap pages (HeapSnapshot.hpp) while that snapshot runs; otherwise
// BattleSnapshot's field restore. The packed BattleSnapshot is the checksum the machines compare either way.
static bool g_heap_restore = true;
static bool g_heap_resource = false;   // also roll back the Resource / Sound heaps (other threads use them)
static bool g_heap_sound = false;
static bool heap_restoring()
{
    return g_heap_restore && heap_snapshot::active();
}
static void sound_after_rewind();   // below, with the sound hooks
static std::map<int, std::vector<uint8_t>> g_latest_saves;   // diagnostics: the latest packed save of each recent frame
// diagnostics (DMC4_RNG_WATCH): the code that drew from the game's random stream in each tick (a write watch on its index)
static bool g_rng_watch = false;
static bool g_speed_watch = false;   // DMC4_WATCH_SPEED: the watch is on sDevil4Main::mGlobalSpeed
static int g_tick_frame = -1;   // the frame run_simulation_tick is simulating (an advance event's), -1 = none
static uint32_t g_tick_serial = 0;   // run_simulation_tick calls so far (diagnostics: which pass a log line is from)
static uint32_t g_tick_input_hash = 0, g_tick_pad_hash[2] = {}, g_tick_raw_hash[2] = {};   // what the tick ran on
static std::vector<uint32_t> g_rng_eips;
static std::map<int, std::vector<uint32_t>> g_rng_by_frame;
static std::vector<std::string> g_rng_stacks;                    // the stacks of draws from DMC4_RNG_STACK's caller
static std::map<int, std::vector<std::string>> g_rng_stacks_by_frame;
static void arm_rng_watch();
static Coop* get_coop_mod();
// diagnostics (DMC4_UNIT_HASH): every unit's bytes hashed in 256-byte chunks at each save, written out at the first desync
// (netdump/units_<pid>_f<n>.bin: per unit its move line and place in it, vtable, bytes, then its chunk hashes, words that
// point into a heap hashed as 0: the two processes' heaps lie at other addresses), to find the first unit that differs
static std::map<int, std::vector<uint32_t>> g_unit_hashes;
static std::vector<uint8_t> g_committed(0x10000);
static void hash_units(std::vector<uint32_t>& out)
{
    out.clear();
    sUnit* su = (sUnit*)*(uintptr_t*)0x00E552CC;
    if (!su) {
        return;
    }
    // words that point at committed memory (outside the exe) are left out of the hashes: the processes' allocations lie
    // elsewhere (committed: 64 KiB granules, also written out with the dumps)
    std::vector<uint8_t>& committed = g_committed;
    std::fill(committed.begin(), committed.end(), 0);
    for (uintptr_t a = 0x10000; a < 0xFFFF0000u;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery((void*)a, &mbi, sizeof mbi)) {
            break;
        }
        const uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT) {
            for (uintptr_t g = (uintptr_t)mbi.BaseAddress >> 16; g <= ((end - 1) >> 16) && g < 0x10000; ++g) {
                committed[g] = 1;
            }
        }
        if (end <= a) {
            break;
        }
        a = end;
    }
    const auto pointer = [&](uint32_t v) { return (v < 0x00400000u || v >= 0x01000000u) && committed[v >> 16]; };
    for (int line = 0; line < 32; ++line) {
        if (line == local_world::kLine) {
            continue;
        }
        int guard = 0;
        for (uintptr_t u = (uintptr_t)su->mMoveLine[line].mTop; u && guard < 4096; u = *(uintptr_t*)(u + 8), ++guard) {
            uintptr_t block = 0;
            uint32_t bytes = 0;
            if (!heap_snapshot::locate(u, block, bytes) || block != u || bytes > 0x100000) {
                continue;
            }
            bytes &= ~3u;
            out.push_back((uint32_t)(line << 16 | guard));
            out.push_back(*(const uint32_t*)u);
            out.push_back(bytes);
            static const char* raw_lines = std::getenv("DMC4_UNIT_RAW");   // "13,15": those lines' words themselves
            char tag[8];
            snprintf(tag, sizeof tag, ",%d,", line);
            static std::string raw_list = raw_lines ? std::string(",") + raw_lines + "," : std::string();
            const bool raw_all = raw_list == ",all," && *(const uint32_t*)u != 0x00BC86E0;   // effects (uDevil4Effect) left out
            if (raw_all || (!raw_list.empty() && raw_list.find(tag) != std::string::npos)) {
                out.back() |= 0x80000000u;
                const uint32_t* w = (const uint32_t*)u;
                for (uint32_t k = 0; k < bytes / 4; ++k) {
                    out.push_back(w[k]);   // raw: players_cmp.py leaves out words that point at memory on both machines
                }
                continue;
            }
            for (uint32_t o = 0; o < bytes; o += 256) {
                uint32_t h = 2166136261u;
                const uint32_t* w = (const uint32_t*)(u + o);
                const uint32_t n = (bytes - o < 256 ? bytes - o : 256) / 4;
                for (uint32_t k = 0; k < n; ++k) {
                    const uint32_t v = pointer(w[k]) ? 0u : w[k];
                    h = (h ^ v) * 16777619u;
                }
                out.push_back(h);
            }
        }
    }
}

// Diagnostics: this machine's latest saves of frames [frame - 40, frame] as netdump/last_<pid>_f<n>.bin, once per session.
static void flush_latest_saves(int frame)
{
    static bool done = false;
    if (done) {
        return;
    }
    done = true;
    CreateDirectoryA("netdump", NULL);
    for (const auto& [f, units] : g_unit_hashes) {
        if (f > frame) {
            continue;
        }
        char name[MAX_PATH];
        sprintf_s(name, "netdump/units_%lu_f%d.bin", GetCurrentProcessId(), f);
        if (FILE* out = std::fopen(name, "wb")) {
            std::fwrite(units.data(), 4, units.size(), out);
            std::fclose(out);
        }
    }
    for (const auto& [f, bytes] : g_latest_saves) {
        if (f < frame - 40 || f > frame) {
            continue;
        }
        char name[MAX_PATH];
        sprintf_s(name, "netdump/last_%lu_f%d.bin", GetCurrentProcessId(), f);
        if (FILE* out = std::fopen(name, "wb")) {
            std::fwrite(bytes.data(), 1, bytes.size(), out);
            std::fclose(out);
        }
        if (f == frame && !g_unit_hashes.empty()) {
            sprintf_s(name, "netdump/committed_%lu.bin", GetCurrentProcessId());
            if (FILE* out = std::fopen(name, "wb")) {
                std::fwrite(g_committed.data(), 1, g_committed.size(), out);
                std::fclose(out);
            }
        }
        const auto draws = g_rng_by_frame.find(f);
        if (draws != g_rng_by_frame.end()) {
            sprintf_s(name, "netdump/rng_%lu_f%d.txt", GetCurrentProcessId(), f);
            if (FILE* out = std::fopen(name, "w")) {
                std::fprintf(out, "dll %08X%c", (unsigned)(uintptr_t)GetModuleHandleA("dinput8.dll"), 10);
                for (const std::string& line : g_rng_stacks_by_frame[f]) {
                    std::fprintf(out, "stack%s%c", line.c_str(), 10);
                }
                for (uint32_t eip : draws->second) {
                    std::fprintf(out, "%08X\n", eip);
                }
                std::fclose(out);
            }
        }
    }
}
static void probe_log(const char* text);   // the probe's log, or netplay_online_<pid>.log in a session

// Simulation state carried across rollback frames: GekkoNet issues a save event every
// checkpoint frame and a load event when it rolls back, and this owns the one snapshot
// the session keeps alive.
struct Devil4State {
    netplay_snapshot::BattleSnapshot snapshot{};

    bool save_game(sDevil4Main* main, GekkoGameEvent* event, SimCtx& ctx) {
        
        const char* reason = nullptr;
        if (!main || !event || event->type != GekkoSaveEvent) {
            spdlog::error("netplay: save_game called with the wrong event type");
            return false;
        }
        // The session's first save runs before any tick: capture the pinned stream - the stage
        // seed - instead of whatever presentation drew while the two machines waited for the
        // handshake, or each side would pin a different stream and diverge from frame 0.
        if (g_tick_rng_fresh) {
            restore_tick_rng();
        }
        arm_rng_watch();
        static const bool shake_log = std::getenv("DMC4_SHAKE_LOG") != nullptr;
        if (shake_log) {
            // with sStylishCount's gauge +0x18C, rank +0x20 and its repetition guard's timers (+0x2C, 30 x 12 bytes)
            char text[900];
            const uintptr_t sc = *(const uintptr_t*)0x00E558CC;
            int at = snprintf(text, sizeof text, "save f%d pass %u input %08X pads %08X %08X raw %08X %08X gauge %.3f rank %d guard",
                (int)event->data.save.frame, g_tick_serial, g_tick_input_hash, g_tick_pad_hash[0], g_tick_pad_hash[1],
                g_tick_raw_hash[0], g_tick_raw_hash[1], sc ? *(const float*)(sc + 0x18C) : 0.0f, sc ? *(const int*)(sc + 0x20) : -1);
            for (int k = 0; sc && k < 30 && at < (int)sizeof text - 12; ++k) {
                if (*(const uint32_t*)(sc + 0x24 + 12 * k)) {
                    const uintptr_t key = *(const uintptr_t*)(sc + 0x28 + 12 * k);
                    uintptr_t block = 0;
                    uint32_t bytes = 0;
                    const char* heap = heap_snapshot::locate(key, block, bytes);
                    at += snprintf(text + at, sizeof text - at, " %d:%u/%.1f/%c+%X/%X", k, *(const uint32_t*)(sc + 0x24 + 12 * k),
                        *(const float*)(sc + 0x2C + 12 * k), heap ? heap[0] : '-', (unsigned)(block ? key - block : key), bytes);
                }
            }
            probe_log(text);
            {
                // each player's camera angle (uPlayer +0x16F8, from his camera's eye and target) and the camera's eye
                char t[300];
                int n = snprintf(t, sizeof t, "cams f%d", (int)event->data.save.frame);
                Coop* coop = get_coop_mod();
                for (int i = 0; i < 2; ++i) {
                    const uintptr_t pl = player_info::player(i);
                    const uintptr_t cam = i == 0 || !(coop && coop->mod_enabled)
                        ? *(const uintptr_t*)(*(const uintptr_t*)0x00E558B8 + 0xD0) : (uintptr_t)coop::players[i].cam;
                    n += snprintf(t + n, sizeof t - n, " p%d %.5f eye %.3f %.3f %.3f perm %02X %02X", i + 1,
                        pl ? *(const float*)(pl + 0x16F8) : 0.0f, cam ? *(const float*)(cam + 0x30) : 0.0f,
                        cam ? *(const float*)(cam + 0x34) : 0.0f, cam ? *(const float*)(cam + 0x38) : 0.0f,
                        pl ? *(const uint8_t*)(pl + 0x1E14) : 0, pl ? *(const uint8_t*)(pl + 0x3148) : 0);
                }
                probe_log(t);
            }
            {
                // every mod's rewound state (sdk/Rollback), hashed with heap pointers left out
                static std::vector<std::pair<const char*, uint32_t>> hashes;
                uintptr_t lo = 0, hi = 0;
                heap_snapshot::heap_span(lo, hi);
                rollback::entry_hashes(hashes, lo, hi);
                std::string t = "mods f" + std::to_string((int)event->data.save.frame);
                char h[16];
                for (const auto& [name, hash] : hashes) {
                    snprintf(h, sizeof h, "%08X", hash);
                    t += std::string(" [") + name + "]" + h;
                }
                probe_log(t.c_str());
            }
            // the em030 shells (vtable 0xBD7458): state +0x1878, sub-state +0x14, position, time step +0x10, hit flags
            sUnit* su = (sUnit*)*(uintptr_t*)0x00E552CC;
            for (int line = 0; su && line < 32; ++line) {
                int guard = 0;
                for (uintptr_t u = (uintptr_t)su->mMoveLine[line].mTop; u && guard < 4096; u = *(uintptr_t*)(u + 8), ++guard) {
                    if (*(const uint32_t*)u != 0x00BD7458u) continue;
                    char t[200];
                    snprintf(t, sizeof t, "shell f%d L%d#%d state %d/%d pos %.3f %.3f %.3f dt %.4f flags %d %d", (int)event->data.save.frame,
                        line, guard, *(const int*)(u + 0x1878), *(const uint8_t*)(u + 0x14), *(const float*)(u + 0x30),
                        *(const float*)(u + 0x34), *(const float*)(u + 0x38), *(const float*)(u + 0x10), *(const uint8_t*)(u + 6285),
                        *(const uint8_t*)(u + 6286));
                    probe_log(t);
                }
            }
        }
        static const bool unit_hash = std::getenv("DMC4_UNIT_HASH") != nullptr;
        static const int unit_upto = std::getenv("DMC4_UNIT_UPTO") ? std::atoi(std::getenv("DMC4_UNIT_UPTO")) : 1 << 30;
        if (unit_hash && (int)event->data.save.frame <= unit_upto) {
            const int f = (int)event->data.save.frame;
            hash_units(g_unit_hashes[f]);
            static const int keep = std::getenv("DMC4_UNIT_KEEP") ? std::atoi(std::getenv("DMC4_UNIT_KEEP")) : 300;
            while (!g_unit_hashes.empty() && g_unit_hashes.begin()->first < f - keep) {
                g_unit_hashes.erase(g_unit_hashes.begin());
            }
        }
        if (g_rng_watch) {
            const int f = (int)event->data.save.frame;
            g_rng_by_frame[f] = g_rng_eips;
            g_rng_stacks_by_frame[f] = g_rng_stacks;
            while (!g_rng_by_frame.empty() && g_rng_by_frame.begin()->first < f - 400) {
                g_rng_by_frame.erase(g_rng_by_frame.begin());
            }
            while (!g_rng_stacks_by_frame.empty() && g_rng_stacks_by_frame.begin()->first < f - 400) {
                g_rng_stacks_by_frame.erase(g_rng_stacks_by_frame.begin());
            }
        }
        if (!netplay_snapshot::capture_battle(main, ctx.g_simTimeUs, snapshot, reason)) {
            spdlog::screen(fmt::format("netplay capture failed: {}", reason), 0xFF4C4CFF);
            return false;
        }
        pin_tick_rng_from_live();
        unsigned char* buffer   = event->data.save.state;
        const uint32_t capacity = netplay_snapshot::state_bytes(); // allocated bytes, not the entry's length
        uint32_t written        = 0;
        if (!netplay_snapshot::pack_into(buffer, capacity, written, snapshot, reason)) {
            spdlog::screen(fmt::format("netplay pack failed: {} (stopped at {} of capacity {}, {}p {}e)", reason, written, capacity,
                               snapshot.player_count, snapshot.enemy_count),
                0xFF4C4CFF);
            return false;
        }
        *event->data.save.state_len = written;
        // Root cursors are real simulation state and remain checksummed. Only the lock-on copies of
        // another unit's position/HP are excluded; those are rebound and re-derived on restore.
        static unsigned char crc_bytes[netplay_snapshot::kWireScratchBytes];
        uint32_t crc_len = 0;
        for (uint32_t i = 0; i < snapshot.enemy_count; ++i) {
            netplay_snapshot::scrub_scratch_gate(snapshot.enemies[i].gate);
        }
        netplay_snapshot::scrub_scratch_camera(snapshot.camera);
        netplay_snapshot::scrub_desync_noise(snapshot);
        if (netplay_snapshot::pack_into(crc_bytes, capacity, crc_len, snapshot, reason)) {
            *event->data.save.checksum = netplay_snapshot::checksum_of(crc_bytes, crc_len);
            // the diagnostic diffs whatever produced the checksum, so it must hold the same bytes
            note_saved_state((int)event->data.save.frame, *event->data.save.checksum, crc_bytes, crc_len);
            {
                // diagnostics: this machine's latest save of each recent frame (the confirmed one once rollbacks passed
                // it), written out at the first desync (flush_latest_saves) to diff against the peer's (netdump_tool diff)
                const int f = (int)event->data.save.frame;
                g_latest_saves[f].assign(crc_bytes, crc_bytes + crc_len);
                while (!g_latest_saves.empty() && g_latest_saves.begin()->first < f - 400) {
                    g_latest_saves.erase(g_latest_saves.begin());
                }
            }
            return save_pages(event);
        }
        *event->data.save.checksum = netplay_snapshot::checksum_of(buffer, written);
        note_saved_state((int)event->data.save.frame, *event->data.save.checksum, buffer, written);
        return save_pages(event);
    }

    static bool save_pages(GekkoGameEvent* event) {
        if (!heap_restoring() || heap_snapshot::save((int)event->data.save.frame)) {
            return true;
        }
        spdlog::screen(fmt::format("netplay heap save failed at f{}: {}", event->data.save.frame,
            heap_snapshot::stats().last_error), 0xFF4C4CFF);
        return false;
    }

    bool load_game(sDevil4Main* main, GekkoGameEvent* event, SimCtx& ctx) {

        const char* reason = nullptr;
        if (!main || !event || event->type != GekkoLoadEvent) {
            spdlog::error("netplay: load_game called with the wrong event type");
            return false;
        }
        if (!netplay_snapshot::unpack_from(event->data.load.state, event->data.load.state_len, snapshot, reason)) {
            spdlog::screen(fmt::format("netplay unpack failed: {}", reason), 0xFF4C4CFF);
            return false;
        }
        if (heap_restoring()) {
            if (!heap_snapshot::load((int)event->data.load.frame)) {
                spdlog::screen(fmt::format("netplay heap load failed at f{}: {}", event->data.load.frame,
                    heap_snapshot::stats().last_error), 0xFF4C4CFF);
                return false;
            }
            netplay_snapshot::apply_timing_state(main, snapshot.timing, ctx.g_simTimeUs);
            pin_tick_rng_from_live();
            {
                // diagnostics: each online load with the loader thread's queue (sResource +0xA050 count, +0xA098 busy)
                static int logged = 0;
                const uintptr_t res = *(const uintptr_t*)0x00E552D0;
                if (logged++ < 20) {
                    char text[160];
                    snprintf(text, sizeof text, "load f%d, loader queue %u busy %u", (int)event->data.load.frame,
                        res ? *(const uint32_t*)(res + 0xA050) : 0u, res ? (unsigned)*(const uint8_t*)(res + 0xA098) : 0u);
                    probe_log(text);
                }
            }
            sound_after_rewind();
            return true;
        }
        probe_tick_determinism(main, snapshot, ctx);
        if (!netplay_snapshot::restore_battle(main, snapshot, ctx.g_simTimeUs, reason)) {
            spdlog::screen(fmt::format("netplay restore failed at f{}: {}", event->data.load.frame, reason), 0xFF4C4CFF);
            return false;
        }
        pin_tick_rng_from_live();
        verify_restore_roundtrip(main, event->data.load.state, event->data.load.state_len, ctx.g_simTimeUs);
        return true;
    }
};


static Devil4State g_devil4_state;

#pragma endregion

#pragma region netplay_base
// One component owns the move loop: CoopOnline selects it from the command line
// (-netplay / -stress, harness/demo args) or from the GUI. The move-loop detour
// dereferences the pointer every presented frame, so both instances exist before the
// first frame; select_mode() below swaps them.
struct NetplayComponent {
    
    SimCtx* ctx = nullptr;

    NetplayComponent() = default;
    virtual ~NetplayComponent() = default;

    virtual void start_session(uint8_t num_players, uint8_t prediction_window) {}
    virtual void end_session() {}

    virtual GekkoSession* get_session() { return nullptr; }

    // One presented frame, in sDevil4Main::move-loop order.
    virtual void before_move(sDevil4Main* main) {}
    virtual void move(sDevil4Main* main) {}
    virtual void after_move(sDevil4Main* main) {}

    virtual void draw_ui() {}
    virtual void on_frame() {}

    // Forwarded from CoopOnline.
    virtual void on_stage_start() {}
    virtual void on_stage_end() {}
    virtual void on_config_load(const utility::Config& cfg) {}
    virtual void on_config_save(utility::Config& cfg) {}
};

static NetplayComponent* g_netplay = nullptr;

// The record the session carries per player per frame: the live pad state (sDevil4Pad
// ::mpInstance 0xE559C4, the same read CoopPads.cpp:65 does). With the input prediction
// window at 0 these are confirmed inputs, so a re-simulation replays exactly what the
// player pressed.
//
// v3 (36 bytes: 4 button words + 4 stick floats + 2 trigger bytes + tail pad): buttons + sticks + the RAW TRIGGER values. mPress is NOT transmitted - it
// is a pure function of the on-bits and the triggers (fill_stand_in's rule, CoopPads.cpp:
// 205: 1.0 per held bit, L2/R2 from the trigger bytes), so write_input_to_pad derives it
// identically on every machine per tick. The 32-byte v1 record never carried the triggers,
// which is what split the machines the moment triggers (attack buttons) were used - and
// transmitting the live mPress instead (v2) desynced from frame 0, because each machine's
// pad update phases it differently. Everything else cPeripheral::update reads (on/trg/rel,
// sticks) is here; mpPad and mVibInfo stay off the wire (pointer / rumble presentation).
struct NetplayInputInfo {
    uint32_t mBtnOn;            // cPadInfo::mBtn.on  - buttons held
    uint32_t mBtnTrg;           // cPadInfo::mBtn.trg - buttons that just pressed
    uint32_t mBtnRel;           // cPadInfo::mBtn.rel - buttons that just released
    uint32_t mBtnRep;           // cPadInfo::mBtn.rep - repeat-held buttons
    float mAnlgX, mAnlgY;       // cPadInfo::mAnlg[0] - left stick
    float mAnlgRX, mAnlgRY;     // cPadInfo::mAnlg[1] - right stick
    uint8_t mTrgL;              // PAD_DATA::Lz - left trigger raw (0-255)
    uint8_t mTrgR;              // PAD_DATA::Rz - right trigger raw
};
static_assert(sizeof(NetplayInputInfo) == 36, "input record layout (34 bytes + 2 tail pad)");

// Test switch DMC4_RANDOM_LIVE for online soak runs: this machine's control plays random face buttons, shoulders, d-pad
// and left stick (never Select / L3 / R3 / Start: the pause menu), seeded by the control so the machines differ.
static void random_online_input(NetplayInputInfo& input, int control)
{
    static uint32_t x = 0x9E3779B9u ^ (uint32_t)(control * 0x85EBCA6Bu + 1);
    static uint32_t previous_on = 0;
    auto next = [] {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x;
    };
    input.mBtnOn = next() & 0xFFF0u;
    input.mBtnTrg = input.mBtnOn & ~previous_on;
    input.mBtnRel = previous_on & ~input.mBtnOn;
    input.mBtnRep = input.mBtnTrg;
    previous_on = input.mBtnOn;
    input.mAnlgX = (float)((int)(next() % 2001) - 1000) / 1000.0f;
    input.mAnlgY = (float)((int)(next() % 2001) - 1000) / 1000.0f;
    input.mTrgL = (uint8_t)next();
    input.mTrgR = (uint8_t)next();
}

static void run_simulation_tick(sDevil4Main* main, GekkoGameEvent* event, SimCtx& ctx);
static void apply_recorded_input(const unsigned char* inputs);
static void fill_live_input(NetplayInputInfo& input, sDevil4Pad* pad);

static SimCtx g_sctx = {}; // Simulation Context

static void fill_live_input_control(NetplayInputInfo& input, int control);
static void apply_recorded_input_control(const NetplayInputInfo& recorded, int control);
static void dump_desync_states(int frame, uint32_t local, uint32_t remote);
static Coop* get_coop_mod();

// Sound is presentation: never rewound (its heap, objects and stream buffers stay as they are). Two things keep it from
// following what a rewind removed:
//  - re-simulated frames queue no sounds: sDevil4Sound::requestSe 0xA47AA0 (stdcall, 8 arguments) is the only writer of
//    the command queue (+0x24, 60 bytes each, count at +0xF024), and a command keeps the unit it follows;
//  - after a rewind, every voice (+0x10850, 0x500 bytes each, count at +0x3CC64) that follows a unit no longer there
//    stops following it (source +0xA0 = 0: the update then keeps its last position). sub_A47E20 called the source's
//    vtable through a freed unit otherwise.
static std::unique_ptr<FunctionHook> g_request_se_hook;
static bool g_sound_from_resim = false;   // debug: let re-simulated frames queue sounds
using RequestSeFn = void(__stdcall*)(int, void*, int, int, void*, int, int, int);
static void __stdcall request_se_detour(int a1, void* res, int a3, int a4, void* a5, int a6, int a7, int a8)
{
    if (g_sctx.g_resimulating && !g_sound_from_resim) {
        return;
    }
    if (res && heap_snapshot::active() && !heap_snapshot::resource_live((uintptr_t)res)) {
        // diagnostics: a sound asked for with a resource that is no longer loaded, and who asked
        static int logged = 0;
        if (logged++ < 10) {
            void* frames[14];
            const USHORT n = RtlCaptureStackBackTrace(0, 14, frames, nullptr);
            static const uintptr_t dll = (uintptr_t)GetModuleHandleA("dinput8.dll");
            char text[400];
            int at = snprintf(text, sizeof text, "requestSe with a resource no longer loaded %08X (first word %08X), stack",
                (unsigned)(uintptr_t)res, *(const uint32_t*)res);
            for (USHORT k = 0; k < n && at < (int)sizeof text - 16; ++k) {
                const uintptr_t a = (uintptr_t)frames[k];
                at += snprintf(text + at, sizeof text - at, a >= dll && a < dll + 0x1500000 ? " d+%X" : " %08X",
                    (unsigned)(a >= dll && a < dll + 0x1500000 ? a - dll : a));
            }
            probe_log(text);
        }
    }
    // a request may draw from the game's random stream (variations), and re-simulated ticks make none: it leaves the stream
    // as it was, or a frame simulated forward and the same frame simulated again would draw differently
    MtRandom saved;
    const bool keep = heap_restoring();
    if (keep) {
        std::memcpy(&saved, mt_random_global(), sizeof saved);
    }
    g_request_se_hook->get_original<std::remove_pointer_t<RequestSeFn>>()(a1, res, a3, a4, a5, a6, a7, a8);
    if (keep) {
        std::memcpy(mt_random_global(), &saved, sizeof saved);
    }
}

static void probe_log(const char* text);

static void units_alive(std::vector<std::pair<uintptr_t, uintptr_t>>& live)
{
    live.clear();
    sUnit* su = (sUnit*)*(uintptr_t*)0x00E552CC;
    if (!su) {
        return;
    }
    for (int line = 0; line < 32; ++line) {
        uintptr_t previous = 0;
        for (uintptr_t u = (uintptr_t)su->mMoveLine[line].mTop; u && live.size() < 100000; previous = u, u = *(uintptr_t*)(u + 8)) {
            if (u < 0x10000 || (u & 3) != 0) {
                // a broken unit list after a rewind (the game would fault on it too): what links to it
                char text[200];
                snprintf(text, sizeof text, "unit list %d broken after a rewind: next %08X after unit %08X (vtable %08X), top %08X",
                    line, (unsigned)u, (unsigned)previous, previous ? *(const uint32_t*)previous : 0u,
                    (unsigned)(uintptr_t)su->mMoveLine[line].mTop);
                probe_log(text);
                char where[160];
                heap_snapshot::describe(previous, where, sizeof where);
                snprintf(text, sizeof text, "  previous %08X: %s", (unsigned)previous, where);
                probe_log(text);
                heap_snapshot::describe(previous + 8, where, sizeof where);
                snprintf(text, sizeof text, "  its next field: %s", where);
                probe_log(text);
                int shown = 0;
                for (uintptr_t c = (uintptr_t)su->mMoveLine[line].mTop; c && c != previous && shown < 24; c = *(uintptr_t*)(c + 8), ++shown) {
                    snprintf(text, sizeof text, "  chain %08X vtable %08X", (unsigned)c, *(const uint32_t*)c);
                    probe_log(text);
                }
                break;
            }
            uintptr_t bytes = (uintptr_t)*(uint32_t*)(u - 16) * 16 - *(uint16_t*)(u - 6);   // MtHeapAllocator::memSize
            if (bytes == 0 || bytes > 0x1000000) {
                bytes = 0x10;
            }
            live.emplace_back(u, u + bytes);
        }
    }
    std::sort(live.begin(), live.end());
}

static uint32_t g_voices_detached = 0;
static void sound_after_rewind()
{
    static constexpr uintptr_t kSoundObjects[] = {0x00E559D8, 0x00E47D38};   // sSoundN, sDevil4Sound
    uintptr_t sound = 0;
    for (uintptr_t global : kSoundObjects) {
        const uintptr_t o = *(const uintptr_t*)global;
        if (o && *(const uintptr_t*)o == 0x00C07544) {
            sound = o;
        }
    }
    if (!sound) {
        return;
    }
    std::vector<std::pair<uintptr_t, uintptr_t>> live;
    units_alive(live);
    auto alive = [&](uintptr_t p) {
        auto it = std::upper_bound(live.begin(), live.end(), std::make_pair(p, UINTPTR_MAX));
        return local_world::contains(p) || (it != live.begin() && p < std::prev(it)->second);
    };
    const uint32_t voices = *(const uint32_t*)(sound + 0x3CC64);
    for (uint32_t i = 0; i < voices && i < 1024; ++i) {
        uintptr_t& source = *(uintptr_t*)(sound + 0x10850 + i * 0x500 + 0xA0);
        if (source && !alive(source)) {
            source = 0;
            ++g_voices_detached;
        }
    }
}

static void begin_heap_snapshot(int rollback_frames)
{
    if (g_heap_restore && !heap_snapshot::begin(rollback_frames + 4)) {
        spdlog::screen(fmt::format("netplay: heap snapshot unavailable ({}), restoring fields instead",
            heap_snapshot::stats().last_error), 0xFF4C4CFF);
    }
}

// Shared event pump for both session types.
struct SessionPumpResult {
    int ticks = 0;
    // advances that moved the session forward (not rollback re-runs); a stress session makes
    // at most one of these per presented frame, so a demo cursor can be driven by it
    int forward_ticks = 0;
    int saves = 0;
    int loads = 0;
    bool rollback_broke = false;
    bool save_refused = false;
    // a restore was refused: the historical rollback Advances are discarded and only the final
    // forward Advance runs (a refused SAVE must not trigger that, the re-simulation continues)
    bool load_refused = false;
};
static SessionPumpResult pump_gekko_events(GekkoSession* session, sDevil4Main* main, SimCtx& ctx, int input_players);
#pragma endregion

#pragma region online_session
struct OnlineComponent : NetplayComponent {
    struct GekkoPlayer {
        int handle { 0 };
        uint16_t port { 0 };
        uint8_t local_delay { 2 };
        bool is_local { false };

        char address_cstr[MAX_PATH] {"127.0.0.1"};
    };

    int num_players = 2;
    uint8_t prediction_window = 8;

    // The control number the human on this machine plays. GekkoNet player i == coop control
    // number i: actors are added in control order, so handle == control == the input block
    // index inside GekkoAdvanceEvent::inputs.
    int local_control = 0;
    bool keyboard_owner_saved = false;
    int saved_keyboard_owner = -1;

    GekkoSession* session = nullptr;
    GekkoPlayer peers[MAX_PLAYERS] = {};

    // readout for draw_ui, updated in move()
    float frames_ahead = 0.0f;
    int avg_ping_ms = 0;

    // Set by the Connect button; the GekkoNet session itself only starts at the next stage
    // load (on_stage_start). A session running in the main menu saves menu states and
    // rollbacks across the stage transition, which is pure desync spam.
    bool connect_requested = false;
    // Deferred to on_frame: on_config_load runs while ModFramework is still constructing, so
    // g_framework (and the mods list get_coop_mod walks) is not usable yet. The first frame
    // is safe and precedes any stage load, so Coop::on_stage_start sees the right player_num.
    bool coop_prep_pending = false;

    OnlineComponent() {
        constexpr uint16_t default_port = 7000;
        for (size_t index = 0; index < MAX_PLAYERS; ++index) {
            peers[index].port = default_port + (uint16_t)index;
        }
        peers[0].is_local = true;
    }

    GekkoPlayer& get_local_player() {
        for (GekkoPlayer& player : peers) {
            if (player.is_local) {
                return player;
            }
        }
        assert(!"no local player set");
        return peers[0];
    }

    void start_session(uint8_t session_players, uint8_t session_prediction_window) override {
        assert(ctx && "Sim context not set!");
        if (session) {
            end_session();
        }
        num_players = session_players < 1 ? 1
            : (session_players > MAX_PLAYERS ? MAX_PLAYERS : session_players);
        prediction_window = session_prediction_window > kMaxPredictionWindow
            ? kMaxPredictionWindow : session_prediction_window;

        // Online play of more than one player runs through the Coop mod: coop control number i
        // is GekkoNet player i, and Coop::on_stage_start (which runs before this mod's) spawns
        // the extra players at the next stage start. The session restarts at every stage, so
        // entering the stage after configuring here is the intended flow.
        prepare_coop();
        if (num_players > 1 && !keyboard_owner_saved) {
            saved_keyboard_owner = coop_pads::keyboard_owner;
            keyboard_owner_saved = true;
            // The keyboard is read directly by cPeripheral::update inside the tick and is not
            // part of the pad input record, so its state differs between the machines and
            // every rollback window would desync. Pads only, per session.
            coop_pads::set_keyboard_owner(-1);
        }

        if (local_control < 0 || local_control >= num_players) {
            local_control = 0;
        }
        for (int index = 0; index < MAX_PLAYERS; ++index) {
            peers[index].is_local = index == local_control;
        }

        if (!gekko_create(&session, GekkoGameSession)) {
            spdlog::error("netplay: gekko_create(GekkoGameSession) failed");
            session = nullptr;
            return;
        }

        // The lockstep clock runs even in the menu (mode None ticks the simulation), so the
        // two machines reach the stage with different accumulated values - the netdump pairs
        // differed in exactly sim_time_us / mTimer / mFrameTimer, every frame, from frame 0.
        // Both sessions start at the stage's first tick, so both clocks start at zero.
        ctx->g_simTimeUs = 0;
        ctx->g_resimulating = false;
        ctx->g_replaying_window = false;
        reset_desync_diff();   // fresh diagnostics per session; also re-arms the first-desync window flush

        // A fresh session's first save must capture the live stream, not a pin left over from
        // the previous one; the stage seed itself is pinned by CoopOnline::on_stage_start after
        // this returns, so the first save still records the seed (see g_tick_rng_fresh).
        g_tick_rng_fresh = false;

        const uint32_t state_size = netplay_snapshot::state_bytes();
        if (state_size == 0) {
            spdlog::error("netplay: snapshot format outgrew its scratch, refusing a session");
            end_session();
            return;
        }
        begin_heap_snapshot(prediction_window);

        GekkoConfig config {0};
        config.desync_detection = true;
        config.input_size = (unsigned int)sizeof(NetplayInputInfo);
        config.state_size = state_size;
        config.max_spectators = 0;
        config.max_input_prediction_window = prediction_window;
        config.num_players = (unsigned int)num_players;
        // every frame is saved (rollback needs a state per frame) because limited_saving
        // defaults to false; GameSession never reads check_distance, only StressSession does

        gekko_start(session, &config);
        GekkoPlayer& local_pl = get_local_player();

        GekkoNetAdapter* adapter = gekko_default_adapter(local_pl.port);
        if (!adapter) {
            // gekko_default_adapter returns null when the UDP bind fails (gekkonet.cpp:313);
            // with no transport Poll returns early and the session would sit there forever
            spdlog::screen(fmt::format("netplay: UDP port {} is already in use, connect cancelled",
                local_pl.port), 0xFF4C4CFF);
            end_session();
            return;
        }
        gekko_net_adapter_set(session, adapter);
        gekko_set_runahead(session, 0); // NOTE(): no runahead :harold:

        for (int index = 0; index < num_players; ++index) {
            GekkoPlayer& player = peers[index];
            if (player.is_local) {
                player.handle = gekko_add_actor(session, GekkoLocalPlayer, nullptr);
                if (player.local_delay > kMaxLocalDelay) {
                    player.local_delay = kMaxLocalDelay;
                }
                gekko_set_local_delay(session, index, player.local_delay);
            }
            else {
                char buffer[MAX_PATH] = { 0 };
                // snprintf reports the untruncated length, so the size check below can actually
                // fire: sprintf would have overrun the buffer before it ran
                const int len = snprintf(buffer, sizeof buffer, "%s:%hu", player.address_cstr, player.port);
                if (len < 0) {
                    fatal_msgbox("Failed to format ip:port for gekkonet asio");
                }
                if (len >= sizeof buffer) {
                    fatal_msgbox("ip:port string format truncated, wtf did you type for an ip address");
                }

                GekkoNetAddress addr = {};
                addr.data = (void*)buffer;
                addr.size = std::strlen(buffer);

                player.handle = gekko_add_actor(session, GekkoRemotePlayer, &addr);
            }
            if (player.handle < 0) {
                // the remote address parser rejects an add (GameSession::AddActor) and then the
                // input block for that control never fills: every Advance would wait for it
                spdlog::error(fmt::format("netplay: gekko_add_actor failed for control {}", index));
                end_session();
                return;
            }
        }
    }

    void end_session() override {
        // the pin belongs to the session that took it: a plain tick after a Disconnect must run
        // on the live stream, not rewind to the last saved one every frame
        clear_tick_rng_pin();
        heap_snapshot::end();
        gekko_default_adapter_destroy();
        gekko_destroy(&session);
        session = nullptr;
        ctx->g_pace_scale = 1.0f;
        frames_ahead = 0.0f;
        avg_ping_ms = 0;
        if (keyboard_owner_saved) {
            coop_pads::set_keyboard_owner(saved_keyboard_owner);
            keyboard_owner_saved = false;
        }
    }

    void before_move(sDevil4Main* main) override {
        if (session) {
            gekko_network_poll(session);
        }
    }

    // One presented frame of online play: submit the local pad, report desyncs, then run
    // whatever GekkoNet answers with (advances include rollback re-runs). The move loop only
    // dispatches this while a session exists; without one it ticks directly in
    // devil4_main_move_loop.
    void move(sDevil4Main* main) override {
        // Adaptive pacing, OnlineSession.cpp handle_frame_time: when this session runs ahead
        // of its slowest peer, stretch the frame deadline ~1.6% so the slower machine catches
        // up instead of this one stalling on missing inputs. Never speeds the pacer up.
        frames_ahead = gekko_frames_ahead(session);
        ctx->g_pace_scale = frames_ahead > 0.5f ? 1.0f / 1.016f : 1.0f;

        // Network readout, OnlineSession.cpp-style but on the screen overlay. Rollback stats
        // count how often the prediction actually had to rewind (and how deep).
        GekkoNetworkStats netstats {};
        float last_ping = 0.0f;
        avg_ping_ms = 0;
        for (int i = 0; i < num_players; ++i) {
            if (peers[i].is_local) {
                continue;
            }
            gekko_network_stats(session, peers[i].handle, &netstats);
            if (netstats.avg_ping > 0.0f) {
                avg_ping_ms = (int)netstats.avg_ping;
                last_ping = netstats.last_ping;
            }
        }
        GekkoRollbackStats rollback {};
        gekko_rollback_stats(session, &rollback);
        {
            static int presented = 0;
            if (++presented % 600 == 0) {
                char text[200];
                snprintf(text, sizeof text, "stats: ping %.1f ms, frames ahead %.2f, rollbacks %u avg %.1f max %u, stalls %u",
                    netstats.avg_ping, frames_ahead, (unsigned)rollback.total_count, rollback.avg_frames,
                    (unsigned)rollback.max_frames, (unsigned)rollback.total_stalls);
                probe_log(text);
            }
        }
        const bool local_pad_ok = coop_pads::connected(local_control);
        spdlog::screen(fmt::format(
            "net | P {:4.1f}ms PA {:4.1f}ms J {:4.1f}ms | S {:5.2f} R {:5.2f} KB/s | FA {:5.2f} | D {} W {} | RB {} avg {:.1f} max {} stalls {} | pad{} {}",
            last_ping, netstats.avg_ping, netstats.jitter, netstats.kb_sent, netstats.kb_received,
            frames_ahead, peers[local_control].local_delay, prediction_window,
            rollback.total_count, rollback.avg_frames, rollback.max_frames, rollback.total_stalls,
            local_control, local_pad_ok ? "ok" : "NOT CONNECTED (Coop panel: player pad index)"),
            0xFFF6FF00);

        // The local human's pad; the peers' inputs arrive through the advance events and are
        // applied to their control's pad before every tick (pump_gekko_events).
        NetplayInputInfo input {};
        fill_live_input_control(input, local_control);
        static const bool random_live = std::getenv("DMC4_RANDOM_LIVE") != nullptr;
        if (random_live) {
            random_online_input(input, local_control);
        }
        gekko_add_local_input(session, local_control, &input);

        int event_count = 0;
        GekkoSessionEvent** events = gekko_session_events(session, &event_count);
        for (int i = 0; i < event_count; ++i) {
            if (events[i]->type == GekkoDesyncDetected) {
                const auto& desync = events[i]->data.desynced;
                // the dump has to run for every event (the pairing is by checksum name); the
                // stress loop caps its screen text, this overlay only ever sees one frame's
                // events, so nothing here needs capping
                spdlog::screen(fmt::format("DESYNC f{} now {:08X} was {:08X}", desync.frame, desync.local_checksum, desync.remote_checksum),
                    0xFF4C4CFF);
                {
                    static int logged = 0;
                    if (logged++ < 200) {
                        char text[96];
                        snprintf(text, sizeof text, "DESYNC f%d local %08X remote %08X", (int)desync.frame,
                            desync.local_checksum, desync.remote_checksum);
                        probe_log(text);
                    }
                }
                dump_desync_states((int)desync.frame, desync.local_checksum, desync.remote_checksum);
                flush_latest_saves((int)desync.frame);
            }
        }

        const SessionPumpResult result = pump_gekko_events(session, main, *ctx, num_players);
        if (result.forward_ticks > 1) {
            // diagnostics: more than one forward tick in a presented frame (the native prelude / tail ran once)
            static int logged = 0;
            if (logged++ < 40) {
                char text[120];
                snprintf(text, sizeof text, "pump: %d forward ticks, %d ticks, %d loads in one frame (last f%d)",
                    result.forward_ticks, result.ticks, result.loads, g_tick_frame);
                probe_log(text);
            }
        }
        if (result.rollback_broke) {
            if (result.save_refused) {
                // A pack that does not fit or a capture that refuses leaves the ring
                // compromised for this window; the pump kept the frame counter aligned.
                spdlog::screen("online: a state save was refused; desyncs likely until the next stage",
                    0xFF4C4CFF);
            } else {
                // A restore refused: a unit appeared or vanished inside the window (a coop revive
                // respawn does exactly that). The ring states no longer match the live unit set;
                // desyncs from here are expected until the next stage restarts the session.
                spdlog::screen("online: restore refused inside the window; desyncs likely until the next stage",
                    0xFF4C4CFF);
            }
        }
    }

    void after_move(sDevil4Main* main) override {}

    void on_frame() override {
        if (coop_prep_pending) {
            coop_prep_pending = false;
            prepare_coop();
        }
    }

    // The extra players exist by now (Coop::on_stage_start runs first); a session started in
    // an earlier stage carries stale saved states, so restart at every stage. Both machines
    // have to be in the same stage - GekkoNet syncs from wherever each side starts. Nothing
    // runs until Connect armed it: no session in the main menu.
    void on_stage_start() override {
        if (!connect_requested) {
            return;
        }
        if (session) {
            end_session();
        }
        start_session((uint8_t)num_players, prediction_window);
    }

    void on_stage_end() override {
        end_session();
    }

    // Launch-line arming: -netplay -plN local|<address> [-plNport <port>], e.g.
    //   A: -netplay -pl0 local -pl0port 7000 -pl1 127.0.0.1 -pl1port 7001
    //   B: -netplay -pl0 127.0.0.1 -pl0port 7000 -pl1 local -pl1port 7001
    // -plN "local" makes this machine play control N; otherwise the token is the peer
    // address of that control. -plNport sets control N's port (defaults 7000+N). Any -plN
    // arms the connect and enables coop for that player count; the session itself starts at
    // the next stage load. (-mutex and -arcade/-character/-difficulty/-mission on the same
    // line are handled by MultipleGameInstances and ArcadeMode.)
    void arm_from_command_line() {
        char value[MAX_PATH] = {};
        int highest = -1;
        int local = -1;
        for (int control = 0; control < MAX_PLAYERS; ++control) {
            char name[16];
            sprintf_s(name, "-pl%d", control);
            if (!utility::get_argument(name, value, sizeof value)) {
                continue;
            }
            highest = control;
            if (_stricmp(value, "local") == 0) {
                local = control;
            }
            else {
                strncpy_s(peers[control].address_cstr, value, _TRUNCATE);
            }
            sprintf_s(name, "-pl%dport", control);
            if (utility::get_argument(name, value, sizeof value)) {
                peers[control].port = (uint16_t)atoi(value);
            }
            sprintf_s(name, "-pl%dchar", control);
            const int c = utility::get_argument(name);
            if (c > -1) {
                coop::players[control].character = c;
            }
        }
        if (highest < 0) {
            return;
        }
        if (local >= 0) {
            local_control = local;
        }
        num_players = highest + 1;
        connect_requested = true;
        coop_prep_pending = true;
        spdlog::screen(fmt::format("netplay: armed from the command line, {} players, local {}",
            num_players, local_control), 0xFF9ACDFF);
    }

    void draw_ui() override {
        // These shape the session's input buffers and actor set: num_players sizes every
        // advance input block and local_control picks which one is submitted, so changing them
        // while a session runs would read past the buffer or aim the local input at a remote
        // handle. The values take effect when the session (re)starts at the next stage.
        if (session) {
            ImGui::BeginDisabled();
        }
        ImGui::InputInt("Number of players", &num_players, 1, 1);
        if (num_players < 1) {
            num_players = 1;
        }
        if (num_players > MAX_PLAYERS) {
            num_players = MAX_PLAYERS;
        }
        ImGui::InputScalar("Prediction window", ImGuiDataType_U8, &prediction_window);
        if (prediction_window > kMaxPredictionWindow) {
            prediction_window = kMaxPredictionWindow;
        }

        int local = local_control;
        if (ImGui::InputInt("This machine plays player #", &local, 1, 1)) {
            local_control = local < 0 ? 0 : (local >= num_players ? num_players - 1 : local);
        }
        ImGui::TextDisabled("Control i == GekkoNet player i. Extra players spawn at the next stage start.");
        ImGui::Text("Configure players:");

        for (int i = 0; i < num_players; ++i) {
            ImGui::PushID(0xC005 + i);
            ImGui::Text("Player: %d%s", i + 1, i == local_control ? "  (this machine)" : "");
            if (i != local_control) {
                ImGui::InputText("address", peers[i].address_cstr, sizeof peers[i].address_cstr);
                ImGui::InputScalar("port", ImGuiDataType_U16, &peers[i].port);
            }
            ImGui::InputScalar("local delay", ImGuiDataType_U8, &peers[i].local_delay);
            if (peers[i].local_delay > kMaxLocalDelay) {
                peers[i].local_delay = kMaxLocalDelay;
            }
            ImGui::PopID();
        }
        if (session) {
            ImGui::EndDisabled();
        }

        if (session) {
            ImGui::Text("ping: %d ms   frames ahead: %.2f", avg_ping_ms, frames_ahead);
            if (ImGui::Button("Disconnect")) {
                connect_requested = false;
                end_session();
            }
        }
        else if (connect_requested) {
            ImGui::Text("Connect armed: the session starts at the next stage load.");
            if (ImGui::Button("Cancel connect")) {
                connect_requested = false;
            }
        }
        else if (ImGui::Button("Connect (session starts at the next stage load)")) {
            connect_requested = true;
            coop_prep_pending = true;
            spdlog::screen("netplay: connect armed, waiting for the next stage load", 0xFF9ACDFF);
        }
    }

    GekkoSession* get_session() override { return session; }

  private:
    // Coop side of online play: control count and mod on, so Coop::on_stage_start spawns the
    // extra players at the stage load. Cheap and idempotent; also run at Connect click time,
    // so arming in the main menu is enough for the stage to spawn the players.
    void prepare_coop() {
        if (num_players <= 1) {
            return;
        }
        if (Coop* coop = get_coop_mod()) {
            coop->player_num = (unsigned int)num_players;
            if (!coop->mod_enabled) {
                // toggle() only installs the patches and preloads the player DTIs; the Coop
                // panel's checkbox is what normally sets mod_enabled, and every Coop detour
                // (and on_stage_start) gates on it. Netplay arms it here instead.
                coop->mod_enabled = true;
                coop->toggle(true);
            }
        } else {
            spdlog::screen("online coop needs the Coop mod registered", 0xFF4C4CFF);
        }
    }
};
#pragma endregion

#pragma region sim_debug
// GekkoNet allocates state_size bytes per saved frame (GekkoLib/src/storage.cpp:16) and hands the
// save event a pointer to the ring entry's state_len (GekkoLib/src/event.cpp:248), which the load
// event also reads as the packed length (event.cpp:232). Writing the packed length back is what
// the load needs, but it also means *state_len stops being the capacity: the ring slot is reused
// check_distance + 2 frames later and would then report the previous frame's shrunk length. The
// capacity has to come from the session config, never from *state_len.

// Desync diagnostics (debug builds only). GekkoNet reports a desync as two CRCs: the ring entry's
// current checksum and the one it recorded in _checksum_history. A frame number is saved more
// than once per presented frame - UpdateRecording (stress_session.cpp:155) issues
// AddStateSaveEvent(current - 1, _replay.PendingState()) next to the ring saves - so keying the
// copies by frame number diffs two saves that were never the two sides of the comparison. Keep
// the bytes of the most recent saves and diff the pair whose checksums are the two the event
// names. Dev builds keep them too (two Release instances over loopback).
#if defined(_DEBUG) || !defined(PUBLIC_BUILD)
// One presented frame re-saves check_distance frames, and a frame's checksum is only compared
// check_distance frames later, so the ring of recent samples has to stay wider than that
// (one eviction window is enough); at the worst-case state size kDesyncSampleSlots samples
// are a few MiB of heap, which is fine for a debug stress test.

// Field naming does not live in this file: netdump_tool decodes the same packs with field tables
// that static_assert against BattleSnapshot.hpp and re-validates the walk against the bytes
// (motion_count / joint_count read from the pack must land on the file size), so it names the exact
// field and prints both sides' values. All this file does is keep the recent packs and hand the
// pair the desync event names to the dump writer.
constexpr int kDesyncSampleSlots = 20;   // the pair an event names is check_distance + 2 frames
                                         // apart; 16 is how far back the first-desync history flush reaches

struct DesyncSample {
    int frame;
    uint32_t checksum;
    std::vector<unsigned char> bytes;
};
static std::vector<DesyncSample> g_desync_samples;
static int g_dump_count = 0;             // netdump files written this process
static constexpr int kDumpLimit = 256;   // desync-every-frame otherwise floods disk
static bool g_dumped_history = false;    // the first desync also flushes the save window


static void reset_desync_diff()
{
    g_desync_samples.clear();
    g_dump_count = 0;
    g_dumped_history = false;
}

static void note_saved_state(int frame, uint32_t checksum, const unsigned char* bytes, uint32_t len)
{
    DesyncSample sample{ frame, checksum, std::vector<unsigned char>{} };
    sample.bytes.assign(bytes, bytes + len);
    g_desync_samples.push_back(std::move(sample));
    if ((int)g_desync_samples.size() > kDesyncSampleSlots) {
        g_desync_samples.erase(g_desync_samples.begin());
    }
}

// Cross-machine state dumps. GekkoNet only exchanges checksums, so the peer's bytes never
// reach this process; instead each machine flushes the saves that produced the two checksums
// the desync event names (note_saved_state keeps the most recent ones) into
// netdump\\f<frame>_c<checksum>.bin. The checksum that matches across the two machines names
// the same pack; the two files with the same frame and different checksums are the desync
// pair - netdump_tool diff names the field and decodes both values; netdump_tool scan pairs them.
// On the FIRST desync of a session the whole recent save window is also flushed, so the first
// diverging frame can be found by pairing the two machines' files for f<N>-1 .. f<N>.
// Bytes are kept in debug and dev builds (note_saved_state), so dumps need such a build on both
// machines with the same state layout.
static void dump_sample(const DesyncSample& sample)
{
    char name[MAX_PATH];
    sprintf_s(name, "netdump\\f%d_c%08X.bin", sample.frame, sample.checksum);
    FILE* f = nullptr;
    if (fopen_s(&f, name, "wb") != 0 || !f) {
        spdlog::screen(fmt::format("netdump: failed to open {}", name), 0xFF4C4CFF);
        return;
    }
    const size_t written = std::fwrite(sample.bytes.data(), 1, sample.bytes.size(), f);
    std::fclose(f);
    ++g_dump_count;
    spdlog::screen(fmt::format("netdump: {} ({} of {} bytes)", name, written,
        sample.bytes.size()), 0xFF9ACDFF);
}

static void dump_desync_states(int frame, uint32_t local, uint32_t remote)
{
    if (g_dump_count >= kDumpLimit) {
        return;
    }
    CreateDirectoryA("netdump", NULL);
    const int window_begin = g_dumped_history ? frame : frame - 16;
    for (const DesyncSample& sample : g_desync_samples) {
        if (sample.frame < window_begin) {
            continue;
        }
        if (sample.frame == frame && sample.checksum != local && sample.checksum != remote) {
            continue;
        }
        dump_sample(sample);
    }
    g_dumped_history = true;
}
#else
static inline void reset_desync_diff() {}
static inline void note_saved_state(int frame, uint32_t checksum, const unsigned char* bytes, uint32_t len) {}
static inline void dump_desync_states(int frame, uint32_t local, uint32_t remote) {}
#endif

// Restore has to be the exact inverse of capture. Pack what a load just applied and compare it
// with the bytes that were loaded: a mismatch is the snapshot's own bug (capture and apply
// disagreeing about a field), while a clean round trip means a DESYNC can only come from the
// tick not reproducing a field. Debug builds only; the static buffers are an 8 MiB wire scratch
// plus one ~4 MiB snapshot.
#ifdef _DEBUG
static void verify_restore_roundtrip(sDevil4Main* main, const unsigned char* loaded, uint32_t loaded_len, uint64_t sim_time)
{
    static netplay_snapshot::BattleSnapshot again{};
    static unsigned char scratch[netplay_snapshot::kWireScratchBytes];
    const char* reason = nullptr;
    if (!netplay_snapshot::capture_battle(main, sim_time, again, reason)) {
        spdlog::screen(fmt::format("restore round trip: capture failed: {}", reason), 0xFF4C4CFF);
        return;
    }
    uint32_t written = 0;
    if (!netplay_snapshot::pack_into(scratch, netplay_snapshot::state_bytes(), written, again, reason)) {
        spdlog::screen(fmt::format("restore round trip: pack failed: {}", reason), 0xFF4C4CFF);
        return;
    }
    const uint32_t len = loaded_len < written ? loaded_len : written;
    uint32_t first_diff = len, differs = 0;
    for (uint32_t i = 0; i < len; ++i) {
        if (loaded[i] != scratch[i]) {
            if (first_diff == len) {
                first_diff = i;
            }
            ++differs;
        }
    }
    if (first_diff == len) {
        if (loaded_len != written) {
            spdlog::screen(fmt::format("RESTORE not lossless: {} bytes loaded, {} captured",
                loaded_len, written), 0xFF4C4CFF);
        }
        return;
    }
    spdlog::screen(fmt::format("RESTORE not lossless at {:#X}: applied {:02X} captured {:02X}, {} of {} bytes",
        first_diff, loaded[first_diff], scratch[first_diff], differs, len), 0xFF4C4CFF);
    spdlog::error("netplay: restore is not the inverse of capture at byte {}", first_diff);
}
#else
static inline void verify_restore_roundtrip(sDevil4Main* main, const unsigned char* loaded, uint32_t loaded_len, uint64_t sim_time) {}
#endif

// sDevil4Main::move keeps running after the block the detour replaces: 0x8AF04C..0x8AF179 is the
// per-frame work of sDevil4Effect, the 0x103C0 timers, sCamera, the emitters, mpDevil4RouteQ,
// sDevil4Sound, the 0x10390 state machine, sDevil4Tool, the uDevil4Model material pass, mpRender,
// sMediator, sSave, sWorkRate and 0x10370 (all identified in the IDB). A frame's state is saved
// *before* its tail runs, so a re-simulated frame needs tail(N) before tick(N+1) or the sequence is
// a different dynamical system - which is why the desyncs were confined to the frames saved before
// the first rollback of a session window and every later frame matched exactly.
//
// The tail belongs to the tick, not to the presented frame: the detour jumps into it only when the
// frame ran a forward tick (g_native_tail_due). A presented frame that stalls on peer input - or
// the connect wait before the first advance, which can last hundreds of frames on the machine
// that loaded the stage first - runs only the presentation systems instead. Without this, every
// zero-tick frame applied another tail to state the snapshot does carry (the 0x10378, 0x103A0 and
// sub_8AFE20 passes move stage units), and two machines whose presented-frame counts drifted apart
// diverged before the first tick. The zero-tick set is exactly what the replay below excludes:
// the desync bisect proved those systems write nothing the checksum covers.
// The tail replay is not a toggle: a frame's state is saved *before* its tail runs, so a
// re-simulated frame needs tail(N) before tick(N+1) or the window is a different dynamical
// system (see the note above). The only knob is the FULL tail, a compile-time bisect switch:
// the scarecrows that never enter battle get no write at all during a rollback (hardware
// watchpoint on their position over a full window: silent) while the live pass moves them,
// so their per-frame motion belongs to one of the remaining tail systems; DESYNC_FULL_TAIL
// replays every one of them, in sDevil4Main::move order, so the watchpoint can bisect them.
// No cfg key: the game rewrites the config on exit, so an edit made while it runs is lost.
#define DESYNC_FULL_TAIL
#ifdef DESYNC_FULL_TAIL
static bool g_replay_tail_full = true;
#else
static bool g_replay_tail_full = false;
#endif
#undef DESYNC_FULL_TAIL

static bool full_tail()
{
    return g_replay_tail_full || heap_restoring();
}

static __forceinline void* main_member(sDevil4Main* main, size_t offset)
{
    return *(void**)((unsigned char*)main + offset);
}

// calls vtable slot `slot_bytes` the way sDevil4Main::move does ([obj] -> [vtable+slot])
static __forceinline void call_system(void* system, size_t slot_bytes)
{
    if (!system) {
        return;
    }
    void** vtable = *(void***)system;
    const uintptr_t fn = *(uintptr_t*)((unsigned char*)vtable + slot_bytes);
    _asm {
        mov ecx, system
        call fn
    }
}

// RNG draw tracing for the rollback tail. Sound/effects/camera all draw from the one global
// MtRandom, so a tail system that consumes draws in the live pass but not in the replay (or the
// reverse) shifts mti for the tick that follows. DMC4_RNG_TRACE=1 names every replayed system
// that moves the draw index, which is the only way to tell a snapshot miss from a presentation
// draw leaking into the shared stream.
static bool rng_trace_enabled()
{
    static int enabled = -1;
    if (enabled < 0) {
        char value[8] = {};
        enabled = GetEnvironmentVariableA("DMC4_RNG_TRACE", value, sizeof value) != 0 ? 1 : 0;
    }
    return enabled != 0;
}

static void call_tail_system(void* system, size_t slot_bytes, const char* name)
{
    const int32_t before = mt_random_mti();
    call_system(system, slot_bytes);
    const int32_t after = mt_random_mti();
    if (before != after && rng_trace_enabled()) {
        spdlog::screen(fmt::format("rng tail {}: mti {} -> {}", name, before, after), 0xFF9ACDFF);
    }
}

static void run_tail_simulation(sDevil4Main* main)
{
    if (!main) {
        return;
    }
    // sDevil4Main::move's tail block is 0x8AF04C..0x8AF13B (read in the IDB): it calls
    // 0x1037C, 0x103C0, 0x10378, 0x10358, 0x103A0, 0x103AC (all vtable +0x18),
    // 0x103A8 (+0x24), sDevil4Sound (sDevil4Sound::mpInstance 0x00E47D38), 0x103A4, 0x10390,
    // 0x1034C, sub_8AFE20(main), 0x10348, 0x10398, 0x103B8, 0x1039C, sub_8AFD60, 0x10370 -
    // and after two of them the presentation helpers sub_AA7F40 (vertex-buffer
    // Unlock on 0x10378), sub_8F33B0 (the 0x10348 present: WaitForSingleObject +
    // D3D present) and sub_8AFD60 (SetThreadExecutionState), which stay out.
    // The replay used to run sWorkRate before sMediator/sSave and never called
    // 0x10378, 0x00E47D38 or the 32-list walk sub_8AFE20 (list heads at main +
    // 0x102F0, stride 24, 0x300 bytes - the pass that moves the stage units the
    // roamers note at :841 lost during a rollback). 0x103A4::move is 0x00484B40,
    // an empty stub, so it needs no replay. Every field the two sides of a save
    // pair disagreed on at the first desync of the online run (netdump f173:
    // model.now_frame_quat/now_frame_pos, facing_direction, the cancel-permission
    // bytes 0x0C51..0x0C5A, enemies[] position/target_position/knockback_*) is
    // written by one of those three, with pad and MtRandom state identical.
    if (full_tail()) {
        call_tail_system(main_member(main, 0x1037C), 0x18, "sDevil4Effect");
    }
    call_tail_system(main_member(main, 0x103C0), 0x18, "timers");
    if (full_tail()) {
        call_tail_system(main_member(main, 0x10378), 0x18, "system 0x10378");
        call_tail_system(main_member(main, 0x10358), 0x18, "sCamera");
        call_tail_system(main_member(main, 0x103A0), 0x18, "emitters");
        call_tail_system(main_member(main, 0x103AC), 0x18, "8-entry list");
    }
    call_tail_system(main_member(main, 0x103A8), 0x24, "route queue");
    if (g_replay_tail_full && !heap_restoring()) {
        // sDevil4Sound::mpInstance (created in sDevil4Main init at 0x8AE1A1 through
        // sub_486E70(0x560, 0x10)); null until then, and call_system dereferences only a
        // non-null system.
        call_tail_system((void*) * (uintptr_t*)0x00E47D38, 0x18, "sDevil4Sound");
    }
    call_tail_system(main_member(main, 0x10390), 0x18, "state machine");
    if (full_tail()) {
        call_tail_system(main_member(main, 0x1034C), 0x18, "sDevil4Tool");
        ((int(__stdcall*)(sDevil4Main*))0x008AFE20)(main);
    }
    call_tail_system(main_member(main, 0x10398), 0x18, "sMediator");
    if (full_tail()) {
        call_tail_system(main_member(main, 0x103B8), 0x18, "sSave");
    }
    call_tail_system(main_member(main, 0x1039C), 0x18, "sWorkRate");
    if (full_tail()) {
        call_tail_system(main_member(main, 0x10370), 0x18, "system 0x10370");
    }
}


// Restoring the same bytes twice and running one tick from each must produce the same bytes. When
// it does not, the tick reads something the snapshot does not carry: the first differing offset
// names the field, and no amount of GekkoNet frame bookkeeping is involved. Runs at most 10 times
// per process.
#ifdef _DEBUG
static void probe_tick_determinism(sDevil4Main* main, const netplay_snapshot::BattleSnapshot& state, SimCtx& ctx)
{
    if (!g_tick_probe_enabled) {
        return;   // the extra ticks mutate globals the snapshot does not carry; off by default
    }
    static int runs = 0;
    if (runs >= 10) {
        return;
    }
    // the first loads are calm; after those, only a crowded state is worth probing
    if (runs >= 2 && state.enemy_count < 2) {
        return;
    }
    ++runs;
    static netplay_snapshot::BattleSnapshot pass_state[2];
    static unsigned char bytes[2][netplay_snapshot::kWireScratchBytes];
    uint32_t len[2] = {0, 0};
    const char* reason = nullptr;
    // The tick reads the pad and the game consumes its trg bits, so both passes must start from the
    // same input or the probe compares two different inputs. NetplayInputInfo's buttons +
    // sticks; the trigger bytes stay zero, identical on both passes.
    NetplayInputInfo pad_record {};
    unsigned char* pad_bytes = (unsigned char*)&pad_record;
    if (sDevil4Pad* pad = *(sDevil4Pad**)netplay_snapshot::kGamePadAddress) {   // sDevil4Pad::mpInstance
        const cPadInfo& info = pad->mPadInfo[0];
        const uint32_t buttons[4] = {info.mBtn.on, info.mBtn.trg, info.mBtn.rel, info.mBtn.rep};
        std::memcpy(pad_bytes, buttons, sizeof buttons);
        std::memcpy(pad_bytes + 16, &info.mAnlg[0], 16);
    }
    for (int pass = 0; pass < 2; ++pass) {
        apply_recorded_input(pad_bytes);
        ctx.g_simTimeUs = state.timing.sim_time_us;
        if (!netplay_snapshot::restore_battle(main, state, ctx.g_simTimeUs, reason)) {
            spdlog::screen(fmt::format("tick determinism probe: restore failed: {}", reason), 0xFF4C4CFF);
            return;
        }
        pin_tick_rng_from_live();
        run_simulation_tick(main, nullptr, ctx);
        if (!netplay_snapshot::capture_battle(main, ctx.g_simTimeUs, pass_state[pass], reason)) {
            spdlog::screen(fmt::format("tick determinism probe: capture failed: {}", reason), 0xFF4C4CFF);
            return;
        }
        for (uint32_t i = 0; i < pass_state[pass].enemy_count; ++i) {
            netplay_snapshot::scrub_scratch_gate(pass_state[pass].enemies[i].gate);
        }
        netplay_snapshot::scrub_scratch_camera(pass_state[pass].camera);
        // same scrub set the save checksum applies, so the probe does not report differences
        // the checksum never sees (per-process continue_words, inactive-slot blocks, camera)
        netplay_snapshot::scrub_desync_noise(pass_state[pass]);
        if (!netplay_snapshot::pack_into(bytes[pass], netplay_snapshot::kWireScratchBytes, len[pass],
                pass_state[pass], reason)) {
            spdlog::screen(fmt::format("tick determinism probe: capture failed: {}", reason), 0xFF4C4CFF);
            return;
        }
    }
    const uint32_t common = len[0] < len[1] ? len[0] : len[1];
    uint32_t first_diff = common, differs = 0;
    for (uint32_t i = 0; i < common; ++i) {
        if (bytes[0][i] != bytes[1][i]) {
            if (first_diff == common) {
                first_diff = i;
            }
            ++differs;
        }
    }
    if (first_diff == common && len[0] == len[1]) {
        spdlog::screen("tick determinism probe: two ticks from the same state agree",
            0xFF9ACDFF);
    } else if (first_diff == common) {
        spdlog::screen(fmt::format("tick determinism probe: lengths {} vs {}", len[0], len[1]),
            0xFF4C4CFF);
    } else {
        spdlog::screen(fmt::format("TICK not a function of state at {:#X}: {:02X} vs {:02X}, {} of {} bytes",
            first_diff, bytes[0][first_diff], bytes[1][first_diff], differs, common), 0xFF4C4CFF);
        spdlog::error("netplay: tick diverges from an identical restored state at byte {}", first_diff);
    }
}
#else
static inline void probe_tick_determinism(sDevil4Main*, const netplay_snapshot::BattleSnapshot&, SimCtx&) {}
#endif

#pragma endregion

#pragma region fpu_stuff
// The simulation runs as an sMain job (sMain::executeJob -> jobProc -> uEnemy::move), so the same
// code can execute on a worker thread. A worker may carry a different MXCSR than the thread that
// ran the previous frame (FTZ / DAZ / rounding), which changes float results without changing any
// game state - exactly the shape of a rollback that is frozen in one window and correct in the
// next. Pin the word the first tick observed, restore it at the start of every tick, and report
// the first time the word or the executing thread actually changed.
// uEm000Base::addGravity is x87 (fld/fstp), so the x87 control word matters as well as MXCSR:
// both are per-thread and both change results without touching game state.
//
// Cross-machine (netdump f154): the first-observed word is per MACHINE - each machine pinned
// its own workers' environment, and every x87 float afterwards carried that difference as a
// sub-ulp divergence (knockback rotation 3rd decimal, style / guard / disaster gauge) from the
// first combat window on. The canonical word is therefore captured ONCE at on_initialize on
// the startup thread - same exe, same value on both machines - and every tick is forced to it.
static uint32_t g_sim_mxcsr = 0;
static uint32_t g_sim_x87cw = 0;
static bool g_sim_env_known = false;
static uint32_t g_sim_thread = 0;
static int g_sim_env_reports = 0;
static uint32_t g_canonical_mxcsr = 0;
static uint32_t g_canonical_x87cw = 0;
static bool g_canonical_env_ready = false;

static void capture_canonical_environment()
{
    g_canonical_mxcsr = _mm_getcsr();
    g_canonical_x87cw = _controlfp(0, 0);
    g_canonical_env_ready = true;
    spdlog::screen(fmt::format("netplay env: mxcsr {:08X} x87 {:04X}",
        g_canonical_mxcsr, g_canonical_x87cw), 0xFF9ACDFF);
}

static void pin_simulation_environment()
{
    // MXCSR bits 0-5 are the sticky exception FLAGS (invalid/denormal/zero/.../precision):
    // purely diagnostic, they cannot change float results, and they accumulate per thread
    // from ordinary inexact math (the laptop's sim threads show 0x1FA5 vs the PC's 0x1FA0
    // from the PE bit alone). Compare and enforce only the bits that do alter results:
    // rounding control, DAZ, FTZ and the exception masks.
    constexpr uint32_t kMxcsrFlagBits = 0x3F;
    const uint32_t csr = _mm_getcsr();
    const uint32_t x87 = _controlfp(0, 0);
    const uint32_t thread = (uint32_t)GetCurrentThreadId();
    if (!g_sim_env_known) {
        g_sim_mxcsr = csr;
        g_sim_x87cw = x87;
        g_sim_thread = thread;
        g_sim_env_known = true;
    }
    const uint32_t want_csr = g_canonical_env_ready ? g_canonical_mxcsr : g_sim_mxcsr;
    const uint32_t want_x87 = g_canonical_env_ready ? g_canonical_x87cw : g_sim_x87cw;
    if ((csr & ~kMxcsrFlagBits) == (want_csr & ~kMxcsrFlagBits) && x87 == want_x87
        && thread == g_sim_thread) {
        return;
    }
    if (g_sim_env_reports < 4) {
        ++g_sim_env_reports;
        spdlog::screen(fmt::format(
            "netplay: sim env changed: mxcsr {:08X}->{:08X} x87 {:04X}->{:04X} thread {}->{}",
            want_csr & ~kMxcsrFlagBits, csr & ~kMxcsrFlagBits, want_x87, x87,
            g_sim_thread, thread), 0xFF9ACDFF);
    }
    if ((csr & ~kMxcsrFlagBits) != (want_csr & ~kMxcsrFlagBits)) {
        // keep the thread's own sticky flags, force the mode bits to canonical
        _mm_setcsr((csr & kMxcsrFlagBits) | (want_csr & ~kMxcsrFlagBits));
    }
    if (x87 != want_x87) {
        _controlfp(want_x87, 0xFFFFFFFFu);
    }
}
#pragma endregion

#pragma region stress_session
// ---------------------------------------------------------------- stress session
//
// GekkoNet's StressSession needs no peers: every frame it rewinds the simulation by
// `check_distance` frames, re-runs it with the inputs recorded for those frames, and
// compares the checksum of the re-run state against the checksum recorded when the
// frame first ran (GekkoLib/src/stress_session.cpp:184 CheckForDesyncs). A mismatch
// raises GekkoDesyncDetected. That is the in-game test for BattleSnapshot: save/load
// has to put the battle back bit for bit, and whatever the snapshot misses shows up
// as a desync at the frame where the re-run first diverged.

static sDevil4Pad* game_pad()
{
    return *(sDevil4Pad**)netplay_snapshot::kGamePadAddress;
}

static void fill_live_input(NetplayInputInfo& input, sDevil4Pad* pad)
{
    input = {};
    if (pad) {
        const cPadInfo& info = pad->mPadInfo[0];
        input.mBtnOn = info.mBtn.on;
        input.mBtnTrg = info.mBtn.trg;
        input.mBtnRel = info.mBtn.rel;
        input.mBtnRep = info.mBtn.rep;
        input.mAnlgX = info.mAnlg[0].x;
        input.mAnlgY = info.mAnlg[0].y;
        input.mAnlgRX = info.mAnlg[1].x;
        input.mAnlgRY = info.mAnlg[1].y;
        // the raw trigger bytes: mPress.L2/R2 are derived from these, and they must travel
        // because the recorded mPress is re-derived per tick on every machine (see v3 note)
        const PAD_DATA& raw = pad->mPad[0].field10_0x15c;
        input.mTrgL = raw.Lz;
        input.mTrgR = raw.Rz;
    }
}

// Control i's pad: CoopPads' stand-in for players 2-4 (and player 1 with an XInput pad
// picked), the game's pad for control 0. Without coop only control 0 exists.
static sDevil4Pad* netplay_pad_for(int control)
{
    if (control == 0) {
        Coop* coop = get_coop_mod();
        if (coop && coop->mod_enabled && coop::players[0].xinput >= 0) {
            return coop_pads::pad_for(0);
        }
        return game_pad();
    }
    Coop* coop = get_coop_mod();
    return coop && coop->mod_enabled ? coop_pads::pad_for(control) : nullptr;
}

static void fill_live_input_control(NetplayInputInfo& input, int control)
{
    fill_live_input(input, netplay_pad_for(control));
}

// The game reads sDevil4Pad inside the tick, so a re-simulated tick has to see the input
// that was recorded for its own frame (GekkoAdvanceEvent::inputs), not whatever the player
// happens to be holding while the session rolls back.
static void write_input_to_pad(const NetplayInputInfo& recorded, sDevil4Pad* pad)
{
    if (!pad) {
        return;
    }
    cPadInfo& info = pad->mPadInfo[0];
    info.mBtn.on = recorded.mBtnOn;
    info.mBtn.trg = recorded.mBtnTrg;
    info.mBtn.rel = recorded.mBtnRel;
    info.mBtn.rep = recorded.mBtnRep;
    info.mAnlg[0].x = recorded.mAnlgX;
    info.mAnlg[0].y = recorded.mAnlgY;
    info.mAnlg[1].x = recorded.mAnlgRX;
    info.mAnlg[1].y = recorded.mAnlgRY;
    // mPress re-derived from the recorded bits + triggers with fill_stand_in's rule, so the
    // tick sees identical values on every machine no matter what each machine's own pad
    // update wrote here this frame (cPeripheral::update memcpy's this block per tick).
    kPressInfo& p = info.mPress;
    p.L2 = recorded.mTrgL * 0.0039215689f;
    p.R2 = recorded.mTrgR * 0.0039215689f;
    const auto on = [held = recorded.mBtnOn](uint32_t bit) { return (held & bit) ? 1.0f : 0.0f; };
    p.Lup = on(0x10);
    p.Lright = on(0x20);
    p.Ldown = on(0x40);
    p.Lleft = on(0x80);
    p.Rup = on(0x1000);
    p.Rright = on(0x2000);
    p.Rdown = on(0x4000);
    p.Rleft = on(0x8000);
    p.L1 = on(0x100);
    p.R1 = on(0x200);
    p.Select = on(0x1);
    p.L3 = on(0x2);
    p.R3 = on(0x4);
}

static void apply_recorded_input(const unsigned char* inputs)
{
    if (!inputs) {
        return;
    }
    write_input_to_pad(*(const NetplayInputInfo*)inputs, game_pad());
}

// Online: every player's recorded input goes to his own control's pad.
static void apply_recorded_input_control(const NetplayInputInfo& recorded, int control)
{
    write_input_to_pad(recorded, netplay_pad_for(control));
}

static Coop* get_coop_mod()
{
    if (!g_framework) {
        return nullptr;   // ModFramework is still constructing (on_config_load runs inside it)
    }
    for (const auto& mod : g_framework->get_mods().get()->get_mods()) {
        if (mod->get_mod_name() == "Coop") {
            return static_cast<Coop*>(mod.get());
        }
    }
    return nullptr;
}

#define DEMO_FILENAME "inputs.demo"

// The stress session rewinds check_distance frames per presented frame; each stored rollback
// state is ~4 MiB and the rewind has to stay well inside the 128-frame input ring
// (InputBuffer::DEFAULT_BUFF_SIZE), so the UI clamps the distance.
static constexpr int kMaxCheckDistance = 64;

// Owns the in-game rollback verification session: GekkoNet's StressSession re-runs
// check_distance frames per presented frame and raises GekkoDesyncDetected when a re-run
// diverges. Also carries the demo record/playback path (-demorec / -demoplay), which
// bypasses the session and ticks straight from the recorded input stream.
struct StressTestComponent : NetplayComponent {
    GekkoSession* session = nullptr;

    int check_distance = 8;   // frames between rewind-and-replay verifications
    int local_delay = 0;
    bool failed = false;      // the session gave up: pack refused or unrestorable every frame
    uint32_t desyncs = 0;
    int restart_streak = 0;   // consecutive fresh-session restarts
    static constexpr int kRestartLimit = 8;

    // demo recording / playback (-demorec / -demoplay)
    bool recording_demo = false;
    bool playback_demo = false;
    bool demo_loaded = false;
    // -stressdemo: a regular stress session whose local input comes from inputs.demo instead of
    // the live pad. Unlike -demoplay this keeps the session, so the stage seed / RNG pin and the
    // rollback verification all run unchanged; only the submitted record's source differs.
    bool demo_stress = false;
    bool demo_exhausted = false;   // the demo ran out; neutral inputs from here on, reported once
    size_t demo_cursor = 0;
    std::vector<NetplayInputInfo> demo_inputs;

    void start_session(uint8_t num_players, uint8_t prediction_window) override;
    void end_session() override;
    void move(sDevil4Main* main) override;
    void after_move(sDevil4Main* main) override;
    void draw_ui() override;
    void on_stage_start() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    
    GekkoSession* get_session() override { return session; }

  private:
    void load_demo();
    void record_frame(sDevil4Main* main, SimCtx& ctx);
    void play_frame(sDevil4Main* main, SimCtx& ctx);
};

void StressTestComponent::start_session(uint8_t num_players, uint8_t prediction_window)
{
    assert(ctx && "Sim context not set!");
    const uint32_t state_size = netplay_snapshot::state_bytes();
    if (state_size == 0) {
        spdlog::error("netplay: snapshot format outgrew its scratch, refusing a stress session");
        failed = true;
        return;
    }

    GekkoSession* new_session = nullptr;
    if (!gekko_create(&new_session, GekkoSessionType::GekkoStressSession)) {
        spdlog::error("netplay: gekko_create(GekkoStressSession) failed");
        failed = true;
        return;
    }

    GekkoConfig config{};
    config.num_players = 1;
    config.input_size = (unsigned int)sizeof(NetplayInputInfo);
    config.state_size = state_size;
    // the reopened session's first save captures the live stream; the stage pin (when this is a
    // stage start) is re-taken by CoopOnline::on_stage_start after this returns
    g_tick_rng_fresh = false;
    reset_desync_diff();
    config.check_distance = (unsigned int)(check_distance < 2 ? 2
        : (check_distance > kMaxCheckDistance ? kMaxCheckDistance : check_distance));
    config.desync_detection = true;
    gekko_start(new_session, &config);

    for (int player = 0; player < (int)config.num_players; ++player) {
        if (gekko_add_actor(new_session, GekkoLocalPlayer, nullptr) < 0) {
            spdlog::error(fmt::format("netplay: gekko_add_actor failed for player {}", player));
            gekko_destroy(&new_session);
            failed = true;
            return;
        }
        if (local_delay > 0) {
            gekko_set_local_delay(new_session, player, (unsigned char)local_delay);
        }
    }
    // No input prediction: the stress test then measures the snapshot alone, and a
    // misprediction cannot masquerade as a desync.
    gekko_set_input_prediction_window(new_session, 0);

    session = new_session;
    begin_heap_snapshot((int)config.check_distance);
    netplay_test::page_session_started((int)config.check_distance);
    spdlog::screen(fmt::format("stress session up: state {} input {} check {} delay {}",
        config.state_size, config.input_size, config.check_distance, local_delay));
}

void StressTestComponent::end_session()
{
    heap_snapshot::end();
    if (session) {
        gekko_destroy(&session);
        session = nullptr;
    }
    // the pin belongs to the session that took it; without this, plain ticks after a given-up
    // session would rewind the stream to the last saved state every frame
    clear_tick_rng_pin();
    failed = false;
    restart_streak = 0;
    ctx->g_resimulating = false;
    ctx->g_replaying_window = false;
}

void StressTestComponent::load_demo()
{
    FILE* f = std::fopen(DEMO_FILENAME, "rb");
    if (!f) {
        MessageBoxA(NULL, "Demo playback requested. Failed to load input recording " DEMO_FILENAME, "DMC4", MB_ICONERROR);
        std::abort();
    }
    std::vector<std::byte> buffer;
    if (std::fseek(f, 0, SEEK_END) == 0) {
        const long size = std::ftell(f);
        if (size > 0 && std::fseek(f, 0, SEEK_SET) == 0) {
            buffer.resize((size_t)size);
            if (std::fread(buffer.data(), 1, buffer.size(), f) != buffer.size()) {
                fatal_msgbox("Demo playback requested. Could not fread whole " DEMO_FILENAME);
            }
            zpp::bits::in in { buffer };
            in(demo_inputs).or_throw();
        }
    }
    std::fclose(f);
    demo_loaded = true;
    demo_cursor = 0;
    demo_exhausted = false;
    spdlog::screen(fmt::format("demo playback: {} frames loaded", demo_inputs.size()));
}

void StressTestComponent::record_frame(sDevil4Main* main, SimCtx& ctx)
{
    NetplayInputInfo& input = demo_inputs.emplace_back();
    fill_live_input(input, game_pad());
    if (input.mBtnOn == 8) {
        std::vector<std::byte> buffer;
        zpp::bits::out out { buffer };
        if (auto rc = out(demo_inputs); zpp::bits::failure(rc)) {
            fatal_msgbox("Demo record requested. Failed to save input recording " DEMO_FILENAME);
        }
        FILE* f = std::fopen(DEMO_FILENAME, "wb");
        if (!f) {
            fatal_msgbox("Demo record requested. failed to open " DEMO_FILENAME);
        }
        if (std::fwrite(buffer.data(), 1, buffer.size(), f) != buffer.size()) {
            fatal_msgbox("Demo record requested. Failed to write all bytes into " DEMO_FILENAME);
        }
        if (std::fclose(f) != 0) {
            fatal_msgbox("Demo record requested. Failed to close " DEMO_FILENAME);
        }
        recording_demo = false;
        spdlog::screen(fmt::format("demo recording saved: {} frames", demo_inputs.size()));
    }
    run_simulation_tick(main, nullptr, ctx);
}

void StressTestComponent::play_frame(sDevil4Main* main, SimCtx& ctx)
{
    if (demo_cursor >= demo_inputs.size()) {
        spdlog::screen("demo playback finished", 0xFF9ACDFF);
        playback_demo = false;
        run_simulation_tick(main, nullptr, ctx);
        return;
    }
    apply_recorded_input((const unsigned char*)&demo_inputs[demo_cursor++]);
    run_simulation_tick(main, nullptr, ctx);
}

void StressTestComponent::move(sDevil4Main* main)
{
    if (recording_demo) {
        // demo frames are plain ticks on the vanilla RNG stream (the stage-start pin would
        // rewind it every tick), so it is cleared and the native tail runs as usual
        clear_tick_rng_pin();
        g_native_tail_due = true;
        record_frame(main, *ctx);
        return;
    }
    if (playback_demo) {
        clear_tick_rng_pin();
        g_native_tail_due = true;
        play_frame(main, *ctx);
        return;
    }
    // The move loop only dispatches this while a session exists; the pump drives the ticks.

    // UpdateSession creates these events by comparing the saves from the preceding presented
    // update. Report them before handling any new SaveEvent so the samples are that exact pair.
    int event_count = 0;
    GekkoSessionEvent** events = gekko_session_events(session, &event_count);
    for (int i = 0; i < event_count; ++i) {
        GekkoSessionEvent* event = events[i];
        if (event->type != GekkoDesyncDetected) {
            continue;
        }
        auto& desync = event->data.desynced;
        ++desyncs;
        netplay_test::desync((int)desync.frame, desync.local_checksum,
            desync.remote_checksum, desyncs);
        dump_desync_states((int)desync.frame, desync.local_checksum, desync.remote_checksum);
        spdlog::screen(fmt::format("DESYNC f{} now {:08X} was {:08X}",
            desync.frame, desync.local_checksum, desync.remote_checksum), 0xFF4C4CFF);
        spdlog::error(fmt::format("netplay: desync at frame {}: re-run {:08X} != saved {:08X}",
            desync.frame, desync.local_checksum, desync.remote_checksum));
    }

    // The stress session's single local input. Gekko's StressSession replays the rollback
    // window from the inputs recorded for those frames (HandleRollback, stress_session.cpp:175)
    // and the forward Advance consumes whatever is recorded for the current frame, so every
    // presented frame has to register an input here - same shape as OnlineComponent::move.
    // -stressdemo replaces the live pad read with the demo's next record: the cursor advances
    // only on actual forward progress (a stalled frame or a reopen re-submits the same record,
    // exactly like a live pad whose state persists across a stall), and once the demo runs out
    // neutral records keep the rollback verification running.
    NetplayInputInfo live_input {};
    if (demo_stress) {
        if (demo_cursor < demo_inputs.size()) {
            live_input = demo_inputs[demo_cursor];
        }
        else if (!demo_exhausted) {
            demo_exhausted = true;
            spdlog::screen("demo stress: inputs.demo exhausted, feeding neutral inputs",
                0xFF9ACDFF);
        }
    }
    else {
        fill_live_input(live_input, game_pad());
    }
    gekko_add_local_input(session, 0, &live_input);

    const SessionPumpResult result = pump_gekko_events(session, main, *ctx, 1);

    if (demo_stress) {
        demo_cursor += (size_t)result.forward_ticks;
        if (demo_cursor > demo_inputs.size()) {
            demo_cursor = demo_inputs.size();
        }
    }

    if (result.rollback_broke) {
        gekko_destroy(&session);
        session = nullptr;
        if (result.save_refused) {
            // A pack that does not fit or a capture that refuses is a real defect, not a state transition.
            spdlog::screen("stress session stopped: a state save was refused", 0xFF4C4CFF);
            spdlog::error("netplay: stress session abandoned after a refused save");
            failed = true;
            return;
        }
        // The live unit set no longer matches what that frame saved - a unit spawned or
        // despawned inside the rollback window, and restore cannot recreate units. Rollback
        // cannot cross that transition, so reopen a session at the live frame and keep the
        // stress test running inside the new window.
        spdlog::screen(fmt::format("stress session reopened at the live frame (restore refused, {} of {})",
            restart_streak + 1, kRestartLimit), 0xFF4C4CFF);
        spdlog::error("netplay: stress session reopened after a refused restore");
        if (++restart_streak > kRestartLimit) {
            spdlog::screen("stress session stopped: unrestorable every frame", 0xFF4C4CFF);
            failed = true;
            return;
        }
        start_session(1, 0);
        return;
    }
    restart_streak = 0;

    spdlog::screen(fmt::format("stress: ticks {} saves {} loads {} desyncs {}",
        result.ticks, result.saves, result.loads, desyncs), 0xFF9ACDFF);
}

void StressTestComponent::after_move(sDevil4Main* main)
{
    if (failed) {
        netplay_test::page_failed();
        netplay_test::complete("session_failed", desyncs);
    } else if (desyncs != 0) {
        netplay_test::page_failed();
        netplay_test::complete("desync", desyncs);
    }
}

void StressTestComponent::draw_ui()
{
    ImGui::InputInt("check distance", &check_distance);
    if (check_distance < 2) {
        check_distance = 2;
    }
    if (check_distance > kMaxCheckDistance) {
        check_distance = kMaxCheckDistance;
    }
    ImGui::InputInt("local delay", &local_delay);
    if (local_delay < 0 || local_delay > 7) {
        local_delay = 0;
    }

    if (!session) {
        if (ImGui::Button("Start stress session")) {
            start_session(1, 0);
            if (failed) {
                netplay_test::complete("session_failed", desyncs);
            }
        }
    }
    else if (ImGui::Button("End stress session")) {
        end_session();
    }
    ImGui::Text("desyncs: %u   restarts: %d", desyncs, restart_streak);
    if (demo_stress) {
        ImGui::Text("demo stress: record %zu of %zu%s", demo_cursor + 1,
            demo_inputs.size(), demo_exhausted ? "  (exhausted, neutral inputs)" : "");
    }
}

void StressTestComponent::on_stage_start()
{
    if (utility::check_argument("-demorec")) {
        recording_demo = true;
        playback_demo = false;
    }
    if (utility::check_argument("-demoplay")) {
        playback_demo = true;
        recording_demo = false;
        if (!demo_loaded) {
            load_demo();
        }
    }
    // -stressdemo: a stress session driven by inputs.demo (the -demorec format), instead of the
    // live pad. Unlike -demoplay it stays inside the GekkoNet session, so the stage seed pin and
    // the rollback verification run unchanged. Restarting the session at every stage replays the
    // demo from frame 0 in the new stage.
    if (utility::check_argument("-stressdemo")) {
        demo_stress = true;
        recording_demo = false;
        playback_demo = false;
        if (!demo_loaded) {
            load_demo();
        }
        demo_cursor = 0;
        demo_exhausted = false;
        spdlog::screen(fmt::format("demo stress: armed ({} frames, check {})", demo_inputs.size(),
            check_distance), 0xFF9ACDFF);
    }
    if (!session && !failed) {
        start_session(1, 0);
        if (failed) {
            netplay_test::complete("session_failed", desyncs);
        }
    }
}

void StressTestComponent::on_config_load(const utility::Config& cfg)
{
    check_distance = cfg.get<int>("coop_online_check_distance").value_or(8);
    if (check_distance < 2) {
        check_distance = 2;
    }
    if (check_distance > kMaxCheckDistance) {
        check_distance = kMaxCheckDistance;
    }
    local_delay = cfg.get<int>("coop_online_local_delay").value_or(0);
    if (local_delay < 0 || local_delay > 7) {
        local_delay = 0;
    }
    g_tick_probe_enabled = cfg.get<bool>("coop_online_tick_probe").value_or(false);
    spdlog::screen(fmt::format("netplay: tail replay on (bisect full {})",
        g_replay_tail_full ? "on" : "off"), 0xFF9ACDFF);
}

void StressTestComponent::on_config_save(utility::Config& cfg)
{
    cfg.set<int>("coop_online_check_distance", check_distance);
    cfg.set<int>("coop_online_local_delay", local_delay);
    cfg.set<bool>("coop_online_tick_probe", g_tick_probe_enabled);
}
#pragma endregion

static StressTestComponent g_stress_component{};
static OnlineComponent g_online_component{};

enum class NetplayMode { None, Online, Stress };
static NetplayMode g_mode = NetplayMode::None;

// Swapping modes ends the old component's session first; the new one starts its own
// session from its UI or from on_stage_start().
static void install_fault_logger();

static void select_mode(NetplayMode mode) {
    if (g_mode == mode) {
        return;
    }
    if (g_netplay) {
        g_netplay->end_session();
    }
    g_netplay = mode == NetplayMode::Online ? (NetplayComponent*)&g_online_component
        : mode == NetplayMode::Stress ? (NetplayComponent*)&g_stress_component
        : nullptr;
    g_mode = mode;
    spdlog::screen(fmt::format("netplay mode: {}",
        mode == NetplayMode::Online ? "online"
        : mode == NetplayMode::Stress ? "stress test" : "none"), 0xFF9ACDFF);

    if (g_netplay) {
        g_netplay->ctx = &g_sctx;
        install_fault_logger();   // faults go to netplay_online_<pid>.log
    }
}

// The harness and demo arguments imply the stress component; -netplay wins for online play.
static void select_mode_from_args() {
    if (utility::check_argument("-netplay")) {
        select_mode(NetplayMode::Online);
        return;
    }
    if (utility::check_argument("-stress") || utility::check_argument("-netplay-run")
        || utility::check_argument("-netplay-log") || utility::check_argument("-demorec")
        || utility::check_argument("-demoplay") || utility::check_argument("-stressdemo")) {
        select_mode(NetplayMode::Stress);
    }
}

#pragma region devil4main_move

// MtRandom lives in sdk/MtRandom.hpp (layout, addresses, the non-standard seeding); the game tick
// stubs below are the only usercalls this file still needs.
// usercall :(
static __forceinline void sCollisionGame__checkInit_sub_47B620(sCollisionGame* col) {
    static constexpr auto func = 0x0047B620;
    _asm {
        mov eax, col
        call func
    }
}
// usercall :(
static __forceinline void sCollisionGame__checkPushMain_sub_47BBA0(sCollisionGame* col) {
    static constexpr auto func = 0x0047BBA0;
    _asm {
        mov eax, col
        call func
    }
}
// usercall :(
static __forceinline void sMediator__clearEnemyList_sub_4937A0(sMediator* med) {
    static constexpr auto func = 0x004937A0;
    _asm {
        mov eax, med
        call func
    }
}
// usercall :(
static __forceinline void sCollisionGame__checkAttackMain_sub_47BC30(sCollisionGame* col) {
    static constexpr auto func = 0x0047BC30;
    _asm {
        mov eax, col
        call func
    }
}

// usercall :( sCollision::setup takes its object in edi (0x95B7C0), not ecx/eax.
static __forceinline void sCollision__setup_sub_95B7C0(void* col) {
    static constexpr auto func = 0x0095B7C0;
    _asm {
        mov edi, col
        call func
    }
}


// The deterministic subset of sMain::move. The outer presented frame calls this once; after a
// rollback load every Advance calls it again from the restored timing state, including the final
// forward Advance. Wall-clock counters and sleeping deliberately stay in the frame pacer.
static void update_simulation_timing(sDevil4Main* main)
{
    main->mNumThreads = main->mNumThreadsLateUpdate;
    main->mSkip = (main->mPause && main->mSkip) ? main->mSkip - 1 : 0;
    // Same pin as run_simulation_tick: sMain_move_fixed calls this outside the tick, with the
    // tail's measured fps still in mFps - mFrameTimer / mFPSActual must not carry it.
    if (main->mFPSlock > 0.0f) {
        main->mFps = main->mFPSlock;
    }
    main->mTimer = g_sctx.g_simTimeUs / 1000;
    main->mFrameTimer = (int32_t)((double)main->mFps * (double)main->mTimer / 1000.0);
    main->mFPSActual = main->mFps;
    main->mDeltaTimeNorm = main->mSpeedGlobal;
}

// NOTE(): hook for deterministic frame timing, static analysis suggests that game can 
// request 30fps lock. 30fps paths are not tested
// this locks to 60, from FramePacer.hpp
static std::unique_ptr<FunctionHook> g_smain_move_timing_hook;
static bool fixed_timing_wanted();
void __fastcall sMain_move_fixed(uint8_t* self, void* unused0)
{
    // the game's own timing unless a netplay mode or the rollback probe needs the fixed clock
    if (!fixed_timing_wanted()) {
        g_smain_move_timing_hook->get_original<decltype(sMain_move_fixed)>()(self, unused0);
        return;
    }
    sDevil4Main* main = (sDevil4Main*)self;
    if (!g_sctx.g_resimulating) {
        // g_pace_scale <= 1: only ever slows the pacer, and only the online component sets it.
        wait_for_next_frame_deadline(main->mFps * g_sctx.g_pace_scale);
    }
    update_simulation_timing(main);
}


// The native sDevil4Main::move prelude that runs BEFORE the unit/collision tick block our
// detour replaces: sCollision::setup (0x8AEEEA), the mediator frame flag + message-queue
// reset, and sArea::move (0x8AEFC6, which dispatches aRoom::move and the room state machine).
// The plain path and a session's first forward frames get these once from the native frame.
// A rollback re-run executes several ticks inside one presented frame, so each re-simulated
// tick (and the forward tick that follows a load) must run its own prelude or the room/collision
// state advances once while the simulation advances check_distance times. Device polling,
// rendering and the presentation draws in the native prelude stay out: inputs come from the
// recorded pad and the tail owns presentation.
static void run_frame_prelude(sDevil4Main* main)
{
    // 0x8AEEC1..0x8AEECC: sDevil4Effect's frame start (vtable +0x20). The other prelude calls are devices
    // (pad 0x1035C, keyboard 0x10388, mouse 0x10384, vibration 0x1038C), loading (sResource 0x10354),
    // rendering (sPrim, the GPU particles, sRender, sShader) or empty (0x1034C +0x24).
    if (heap_restoring()) {
        call_system(main_member(main, 0x1037C), 0x20);
        // 0x8AEECE..0x8AEED4: sPrim's per-frame reset (sub_A34C00, edi = sPrim). Effects submit primitives during the tick;
        // without the reset the frames simulated again kept theirs, and the next drawn frame drew primitives of particles a
        // rewind had already taken away.
        if (void* prim = main_member(main, 0x10360)) {
            const uintptr_t reset = 0x00A34C00;
            __asm {
                mov edi, prim
                call reset
            }
        }
    }
    // 0x8AEEDF..0x8AEEEA: mpCollision+0x179 = 1, then sCollision::setup (edi = the object).
    if (void* col = main->mpD4Collision) {
        *(uint8_t*)((unsigned char*)col + 0x179) = 1;
        sCollision__setup_sub_95B7C0(col);
    }
    // sMediator member_0x468 is a BYTE cleared to 0 (0x8AEF12), under the mediator CS in the
    // native path; the session tick is single-threaded with audio frozen, so a plain store is safe.
    if (sMediator* med = main->mpMediator) {
        *(uint8_t*)((unsigned char*)med + 0x468) = 0;
    }
    if (void* msg = main_member(main, 0x103A0)) {
        unsigned char* entry = (unsigned char*)msg + 872;
        for (int i = 0; i < 12; ++i) {
            *(uint32_t*)entry = 0;
            entry += 832;
        }
    }
    call_system(main_member(main, 0x10374), 0x18);   // sArea::move
}

static bool g_smain_prelude_enabled = false;
static bool g_probe_replay = false;   // the rollback probe's re-run ticks (no GekkoNet event)
static bool g_probe_coop = false;     // test switch DMC4_PROBE_COOP: a second player with random input
// One simulation tick, shared by the plain path and by every GekkoNet advance event
// (rollback included), so a re-simulated tick runs exactly the same code as the
// original pass. The lockstep clock belongs to the tick: HandleRollback (stress_session.cpp:164)
// rewinds the session by check_distance frames with one load event and then issues
// check_distance - 1 rollback advances plus one forward advance, so a presented frame
// runs check_distance ticks and only makes one frame of forward progress. Advancing the
// clock once per presented frame instead of once per tick loses check_distance - 1 frames
// every frame, which is what froze the in-game clock.
// Advance-event inputs are applied by the CALLER (pump_gekko_events, per control online);
// this function never touches the pad, so a re-simulated tick sees exactly what the pump wrote.
// Files a tick asks for are read by the loader thread and handed over by sResource's step in the frame prelude, once per
// presented frame: a unit polling for one saw it on another tick on each machine (player 2's motion files after his
// spawn: one machine played the move's 101 frames, the other a 14-frame stand-in). Online, every tick ends with the
// loader done (sResource +0xA050 queued, +0xA098 busy) and the step run, so a file asked for on tick N is there on tick
// N + 1 on every machine.
static void settle_loads(sDevil4Main* main)
{
    if (!g_netplay || !g_netplay->get_session()) {
        return;
    }
    const uintptr_t res = *(const uintptr_t*)0x00E552D0;   // sDevil4Resource::mpInstance
    if (!res || (uintptr_t)main_member(main, 0x10354) != res) {
        return;
    }
    const auto pending = [res] {
        return *(const volatile uint32_t*)(res + 0xA050) != 0 || *(const volatile uint8_t*)(res + 0xA098) != 0;
    };
    if (!pending()) {
        return;
    }
    const DWORD start = GetTickCount();
    while (pending() && GetTickCount() - start < 3000) {
        Sleep(0);
    }
    call_system((void*)res, 0x18);
}

static void run_simulation_tick(sDevil4Main* main, GekkoGameEvent* event, SimCtx& ctx)
{
    g_rng_eips.clear();
    g_rng_stacks.clear();
    g_tick_frame = event ? (int)event->data.adv.frame : -1;
    ++g_tick_serial;
    {
        // diagnostics: the inputs this tick runs on and the pads they were written to (FNV)
        static const bool shake_log = std::getenv("DMC4_SHAKE_LOG") != nullptr;
        const auto fnv = [](const void* p, size_t n) {
            uint32_t h = 2166136261u;
            for (size_t k = 0; k < n; ++k) h = (h ^ ((const uint8_t*)p)[k]) * 16777619u;
            return h;
        };
        if (shake_log && event) {
            g_tick_input_hash = fnv(event->data.adv.inputs, 2 * sizeof(NetplayInputInfo));
            for (int c = 0; c < 2; ++c) {
                sDevil4Pad* pad = netplay_pad_for(c);
                // player 1's pad: its header (to mBtn), buttons to presses, sensor and rumble apart
                const cPadInfo* pi = pad ? &pad->mPadInfo[0] : nullptr;
                const size_t head = pi ? (size_t)((const char*)&pi->mBtn - (const char*)pi) : 0;
                const size_t sensor = pi ? (size_t)((const char*)&pi->mSensor - (const char*)pi) : 0;
                const size_t vib = pi ? (size_t)((const char*)&pi->mVibInfo - (const char*)pi) : 0;
                g_tick_pad_hash[c] = pi ? fnv(pi, sizeof *pi) : 0;
                g_tick_raw_hash[c] = pi && c == 0 ? (fnv(pi, head) & 0xFF000000u) | (fnv((const char*)pi + head, sensor - head) & 0xFF0000u) |
                    (fnv((const char*)pi + sensor, vib - sensor) & 0xFF00u) | (fnv((const char*)pi + vib, sizeof *pi - vib) & 0xFFu) : 0;
            }
        }
    }
    restore_tick_rng();
    pin_simulation_environment();
    // The measured fps is frame-time smoothing, per machine, and it feeds both the clock
    // increment below and the captured timing state (timing.fps on the wire). Only the locked
    // rate may drive a lockstep clock - both machines must run the same FPS lock setting.
    if (main->mFPSlock > 0.0f) {
        main->mFps = main->mFPSlock;
    }
    const bool replaying = (event && ctx.g_replaying_window) || g_probe_replay;
    const bool prelude = replaying && (g_smain_prelude_enabled || heap_restoring());
    if (prelude) {
        // 0x8AEE90 / 0x8AEE9B: sMediator applies pending frame-rate and frame-wait requests before sMain::move
        if (sMediator* med = main->mpMediator) {
            ((void(__thiscall*)(sMediator*))0x00493D10)(med);
            ((void(__thiscall*)(sMediator*))0x00493E20)(med);
        }
    }
    if (replaying) {
        update_simulation_timing(main);
    }
    const float fps = main->mFps;
    ctx.g_simTimeUs += (fps == 60.0f) ? 16667 : (uint64_t)(1000000.0 / fps);

    if (prelude) {
        run_frame_prelude(main);
    }
    // the mods' per-tick work (Coop's tick), forward and re-simulated alike: after the prelude, as on a forward tick
    frame_hooks::tick_start();

    sCollisionGame__checkInit_sub_47B620(main->mpCollisionGame);
    main->mpUnit->moveExForAll();
    main->mpD4Collision->move();
    sCollisionGame__checkPushMain_sub_47BBA0(main->mpCollisionGame);
    sMediator__clearEnemyList_sub_4937A0(main->mpMediator);
    for (int w = 0; w < 0x20; ++w) {
        main->mpUnit->sync0(w);
    }
    main->mpUnit->moveAfter();
    sCollisionGame__checkAttackMain_sub_47BC30(main->mpCollisionGame);
    // the native block's FRAME_END site (0x8AF03F) is skipped with it: the debug server and plugins run here,
    // once per forward tick
    if (!ctx.g_resimulating) {
        {
            // forward ticks only: what runs here (mods' frame-end work, plugins, the debug server) must not draw from the
            // game's random stream, or the same frame simulated again would draw differently
            MtRandom saved;
            std::memcpy(&saved, mt_random_global(), sizeof saved);
            frame_hooks::frame_end();
            std::memcpy(mt_random_global(), &saved, sizeof saved);
        }
        local_world::tick();   // the local units (visual mods), never re-simulated: after the forward tick, before the draw
    }
    call_system(main->mpD4Collision, 0x20);   // original 0x8AF03F post-attack sync pass
    settle_loads(main);
    // the tick consumed the pinned stream: the next save must capture the live state again
    g_tick_rng_fresh = false;
}

// Shared GekkoNet event pump for both session types: drain one presented frame's
// save/load/advance events. Desync session-events are read by the caller first so the
// diagnostic samples are the exact pair (see StressTestComponent::move). The pump never
// destroys the session; the caller decides what a broken rollback means. `input_players`:
// 1 = the stress session's single input on the game pad; N = an online session, whose
// advance inputs are N concatenated NetplayInputInfo blocks, block i = control i.
static SessionPumpResult pump_gekko_events(GekkoSession* session, sDevil4Main* main, SimCtx& ctx, int input_players)
{
    SessionPumpResult result{};

    int count = 0;
    GekkoGameEvent** updates = gekko_update_session(session, &count);

    bool skip_rollback_save = false;
    for (int i = 0; i < count; ++i) {
        GekkoGameEvent* event = updates[i];
        switch (event->type) {
        case GekkoSaveEvent:
            if (result.rollback_broke && skip_rollback_save) {
                break;
            }
            if (result.save_refused) {
                // The ring is compromised for this window: keep the re-simulation on the
                // original timeline (its tail included) and keep advancing, so the game's frame
                // counter stays aligned with GekkoNet's - only the state writes stop, or later
                // loads would restore a stale slot. Not calling save_game again avoids error
                // spam: every further save would refuse the same way.
                if (ctx.g_resimulating) {
                    run_tail_simulation(main);
                }
                break;
            }
            if (!g_devil4_state.save_game(main, event, ctx)) {
                // A missing save leaves the ring slot stale: the next load of that frame would
                // restore whatever the previous occupant left behind.
                result.save_refused = true;
                result.rollback_broke = true;
                if (ctx.g_resimulating) {
                    run_tail_simulation(main);
                }
            } else {
                ++result.saves;
                netplay_test::page_saved((int)event->data.save.frame, *event->data.save.checksum);
                if (ctx.g_resimulating) {
                    // Gekko emits the advance then its save (stress_session.cpp:175), and the
                    // original pass runs its tail only after the detour returns - so the order a
                    // frame really saw was tick(N) -> save(N) -> tail(N). Running the tail here,
                    // after the pack, is the only place that reproduces it: the re-run save then
                    // holds the pre-tail state, exactly like the save it is compared against.
                    run_tail_simulation(main);
                }
            }
            break;
        case GekkoLoadEvent:
            if (!g_devil4_state.load_game(main, event, ctx)) {
                // Historical events are suppressed below; only the final forward Advance runs.
                result.rollback_broke = true;
                result.load_refused = true;
                ctx.g_replaying_window = false;
            } else {
                ctx.g_replaying_window = true;
                ++result.loads;
                netplay_test::page_loaded((int)event->data.load.frame, true);
                // The loaded state is pre-tail, including the pad as that frame's tick left it.
                run_tail_simulation(main);
            }
            break;
        case GekkoAdvanceEvent:
            ctx.g_resimulating = event->data.adv.rolling_back;
            if (!event->data.adv.rolling_back) {
                // the native tail belongs to the forward tick: re-simulated frames get theirs
                // from the save events above, and a stalled frame (no forward Advance) gets no
                // tail at all - see g_native_tail_due
                g_native_tail_due = true;
                ++result.forward_ticks;
            }
            skip_rollback_save = result.load_refused && ctx.g_resimulating;
            if (skip_rollback_save) {
                // restore_battle refused before writing: discard historical Advances/Saves and
                // execute only the final forward Advance once from the still-live state.
                break;
            }
            if (input_players > 1) {
                const NetplayInputInfo* blocks = (const NetplayInputInfo*)event->data.adv.inputs;
                for (int player = 0; player < input_players; ++player) {
                    apply_recorded_input_control(blocks[player], player);
                }
            } else {
                apply_recorded_input(event->data.adv.inputs);
            }
            ++result.ticks;
            netplay_test::page_advance((int)event->data.adv.frame, event->data.adv.rolling_back,
                !event->data.adv.rolling_back);
            run_simulation_tick(main, event, ctx);
            // the frame's tail runs after its save event (see GekkoSaveEvent), never here
            break;
        default:
            break;
        }
    }
    ctx.g_resimulating = false;
    ctx.g_replaying_window = false;
    return result;
}


#pragma region rollback_probe
// Rollback probe, on one machine without a session: every `interval` frames the last `depth` frames are
// simulated again from the heap snapshot, first with made-up input (a mispredicted run whose attacks, jumps
// and shots the rewind has to undo), then with the input they really had, and heap_snapshot's verify compares
// every page the window wrote with the original run. GekkoNet's stress session can't find this kind of miss:
// it re-simulates with the same input. Results also go to netplay_probe.log in the game folder.
struct RollbackProbe {
    static constexpr int kHistory = 128;
    bool enabled = false;
    bool running = false;
    bool made_up_first = true;
    int depth = 8;
    int interval = 30;
    int frame = 0;              // the next tick
    int last_probe = 0;
    uintptr_t room = 0;         // the room the snapshot belongs to: a rewind can't cross a load
    int settled = 0;            // frames in that room with a player
    bool replay_next = false;   // the forward tick after a probe replays its prelude: the rewind undid it
    uint32_t runs = 0, failures = 0, skipped = 0;
    uint32_t rng = 0x2545F491u;
    heap_snapshot::VerifyResult last{};
    NetplayInputInfo inputs[kHistory]{};
    NetplayInputInfo inputs2[kHistory]{};   // player 2 (DMC4_PROBE_COOP)
    netplay_snapshot::TimingState timing[kHistory]{};
    std::vector<uint8_t> checked[kHistory];   // the checksummed state after each forward tick
    float trace[kHistory][2][10]{};          // per player: what the stick direction is made of, after each forward tick
    std::vector<uint8_t> cam_block[kHistory];  // player 2's camera unit, after each forward tick
    uint32_t checked_differ = 0;              // replays whose checksummed state left the forward run's
};
static RollbackProbe g_probe;

// The probe's log, or in an online session one per process (two instances share the game folder in loopback tests).
static void probe_log(const char* text)
{
    char name[64] = "netplay_probe.log";
    if (g_netplay) {
        snprintf(name, sizeof name, "netplay_online_%lu.log", GetCurrentProcessId());
    }
    if (FILE* f = std::fopen(name, "a")) {
        std::fputs(text, f);
        std::fputc('\n', f);
        std::fclose(f);
    }
}

static volatile LONG g_probe_faults = 0;
static const char* g_probe_phase = "forward";   // what the probe is simulating (fault / watch logs)
static int g_probe_tick = 0;
static uintptr_t g_watch_address = 0;           // test switch DMC4_WATCH=<hex offset in the Unit heap>: writes logged
static volatile LONG g_watch_hits = 0;
constexpr DWORD kArmWatch = 0xE0D40001;

static void probe_log(const char* text);

// Return addresses on the stack (the game's code and this DLL's), for the watch log.
static void stack_addresses(const CONTEXT& c, char* out, size_t n)
{
    static const uintptr_t dll = (uintptr_t)GetModuleHandleA("dinput8.dll");
    size_t at = 0;
    int found = 0;
    const uint32_t* sp = (const uint32_t*)c.Esp;
    for (int i = 0; i < 512 && found < 10 && at + 12 < n; ++i) {
        const uint32_t v = sp[i];
        const bool game = v >= 0x00401000 && v < 0x00B85000;
        const bool ours = v >= dll && v < dll + 0x1500000;
        if (game || ours) {
            at += snprintf(out + at, n - at, ours ? " d+%X" : " %08X", ours ? (unsigned)(v - dll) : (unsigned)v);
            ++found;
        }
    }
    out[at] = 0;
}

// The first faults while the probe runs: where each register points (heap, block), for the probe log.
static LONG CALLBACK probe_fault_logger(EXCEPTION_POINTERS* info)
{
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    CONTEXT& c = *info->ContextRecord;
    if (code == kArmWatch) {
        c.Dr0 = g_watch_address;
        c.Dr7 = 1u | (1u << 16) | (3u << 18);   // L0, break on write, 4 bytes
        c.Dr6 = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && (c.Dr6 & 1) && g_speed_watch) {
        // DMC4_WATCH_SPEED: sDevil4Main's mGlobalSpeed (+0x74) changed: by what, on which tick
        c.Dr6 = 0;
        static uint32_t last = 0;
        const uint32_t now = *(const uint32_t*)g_watch_address;
        if (now != last && InterlockedIncrement(&g_watch_hits) <= 3000) {
            last = now;
            char stack[200];
            stack_addresses(c, stack, sizeof stack);
            float v;
            std::memcpy(&v, &now, 4);
            char line[320];
            snprintf(line, sizeof line, "speed f%d%s pass %u = %.6f at %08X, stack%s", g_tick_frame,
                g_sctx.g_resimulating ? " (again)" : "", g_tick_serial, v, (unsigned)c.Eip, stack);
            probe_log(line);
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && (c.Dr6 & 1) && g_rng_watch) {
        c.Dr6 = 0;
        if (g_rng_eips.size() < 8192) {
            // MtRandom::nextTable 0x4055E0 is a leaf: after its write the caller is at [esp]
            g_rng_eips.push_back(c.Eip == 0x00405603 ? *(const uint32_t*)c.Esp : (uint32_t)c.Eip);
        }
        static const uint32_t stack_of = std::getenv("DMC4_RNG_STACK") ? std::strtoul(std::getenv("DMC4_RNG_STACK"), nullptr, 16) : 0;
        if (stack_of && c.Eip == 0x00405603 && *(const uint32_t*)c.Esp == stack_of && g_rng_stacks.size() < 64) {
            char stack[200];
            stack_addresses(c, stack, sizeof stack);
            g_rng_stacks.emplace_back(stack);
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && (c.Dr6 & 1)) {
        c.Dr6 = 0;
        static const int watch_from = std::getenv("DMC4_WATCH_FROM") ? std::atoi(std::getenv("DMC4_WATCH_FROM")) : 0;
        if (g_probe.frame >= watch_from && InterlockedIncrement(&g_watch_hits) <= 3000) {
            char stack[160];
            stack_addresses(c, stack, sizeof stack);
            char line[300];
            snprintf(line, sizeof line, "watch: %08X now %08X at %08X, %s tick %d, probe frame %d, stack%s",
                (unsigned)g_watch_address, *(const uint32_t*)g_watch_address, (unsigned)c.Eip, g_probe_phase, g_probe_tick,
                g_probe.frame, stack);
            probe_log(line);
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if ((!g_probe.running && !g_netplay) || code != EXCEPTION_ACCESS_VIOLATION || InterlockedIncrement(&g_probe_faults) > 2) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    char line[200];
    snprintf(line, sizeof line, "fault at %08X reading %08X, %s tick %d, probe frame %d", (unsigned)c.Eip,
        (unsigned)info->ExceptionRecord->ExceptionInformation[1], g_probe_phase, g_probe_tick, g_probe.frame);
    probe_log(line);
    const char* names[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"};
    const DWORD values[] = {c.Eax, c.Ebx, c.Ecx, c.Edx, c.Esi, c.Edi, c.Ebp};
    for (int i = 0; i < 7; ++i) {
        uintptr_t block = 0;
        uint32_t bytes = 0;
        const char* heap = heap_snapshot::locate(values[i], block, bytes);
        if (!heap) {
            continue;
        }
        snprintf(line, sizeof line, "  %s %08X %s: block %08X +%X of %X, first word %08X", names[i], (unsigned)values[i],
            heap, (unsigned)block, (unsigned)(block ? values[i] - block : 0), bytes, block ? *(const uint32_t*)block : 0u);
        probe_log(line);
        if (block && bytes <= 0x100 && std::strcmp(heap, "Resource") == 0) {
            // a small Resource-heap object (a cResource): its words and its path
            const uint32_t* w = (const uint32_t*)block;
            for (uint32_t k = 0; k < bytes / 4; k += 8) {
                snprintf(line, sizeof line, "    +%02X %08X %08X %08X %08X %08X %08X %08X %08X", k * 4, w[k], w[k + 1],
                    w[k + 2], w[k + 3], w[k + 4], w[k + 5], w[k + 6], w[k + 7]);
                probe_log(line);
            }
            char path[80] = {};
            std::memcpy(path, (const char*)block + 0x0C, 64);
            for (char& ch : path) {
                if (ch && (ch < 32 || ch > 126)) ch = '?';
            }
            snprintf(line, sizeof line, "    path at +0C: %s", path);
            probe_log(line);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void arm_rng_watch()
{
    static bool armed = false;
    if (!armed && std::getenv("DMC4_WATCH_SPEED")) {
        armed = true;
        install_fault_logger();
        g_watch_address = *(const uintptr_t*)0x00E5574C + 0x74;   // sDevil4Main::mpInstance, mGlobalSpeed
        g_speed_watch = true;
        RaiseException(kArmWatch, 0, 0, nullptr);
        return;
    }
    if (armed || !std::getenv("DMC4_RNG_WATCH")) {
        return;
    }
    armed = true;
    install_fault_logger();
    g_watch_address = 0x00EAD070;   // MtRandom (0xEAC6B0) mti, +0x9C0
    g_rng_watch = true;
    RaiseException(kArmWatch, 0, 0, nullptr);
}

static void install_fault_logger()
{
    static PVOID handler = AddVectoredExceptionHandler(1, probe_fault_logger);
    (void)handler;
}

static void probe_stop()
{
    if (g_probe.running) {
        heap_snapshot::end();
        g_probe.running = false;
        clear_tick_rng_pin();
    }
}

// Buttons, the left stick and the triggers at random: face buttons, shoulders and the d-pad, never
// Select / L3 / R3 / Start (bits 0-3: the pause menu).
static NetplayInputInfo probe_made_up_input(const NetplayInputInfo& real, uint32_t& previous_on)
{
    auto next = [] {
        uint32_t& x = g_probe.rng;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x;
    };
    NetplayInputInfo input = real;
    input.mBtnOn = next() & 0xFFF0u;
    input.mBtnTrg = input.mBtnOn & ~previous_on;
    input.mBtnRel = previous_on & ~input.mBtnOn;
    input.mBtnRep = input.mBtnTrg;
    previous_on = input.mBtnOn;
    input.mAnlgX = (float)((int)(next() % 2001) - 1000) / 1000.0f;
    input.mAnlgY = (float)((int)(next() % 2001) - 1000) / 1000.0f;
    input.mTrgL = (uint8_t)next();
    input.mTrgR = (uint8_t)next();
    return input;
}

// Diagnostics: the bytes the online checksum is taken from (save_game's scrubbed pack), for the probe's comparison.
static bool pack_checked(sDevil4Main* main, std::vector<uint8_t>& out)
{
    static netplay_snapshot::BattleSnapshot snap;
    static unsigned char bytes[netplay_snapshot::kWireScratchBytes];
    const char* reason = nullptr;
    out.clear();
    if (!netplay_snapshot::capture_battle(main, g_sctx.g_simTimeUs, snap, reason)) {
        return false;
    }
    for (uint32_t i = 0; i < snap.enemy_count; ++i) {
        netplay_snapshot::scrub_scratch_gate(snap.enemies[i].gate);
    }
    netplay_snapshot::scrub_scratch_camera(snap.camera);
    netplay_snapshot::scrub_desync_noise(snap);
    uint32_t len = 0;
    if (!netplay_snapshot::pack_into(bytes, netplay_snapshot::state_bytes(), len, snap, reason)) {
        return false;
    }
    out.assign(bytes, bytes + len);
    return true;
}

// The real-input replay of frame k against its forward run: the first replay that differs writes both packs to
// netdump/probe_fwd_f<k>.bin / probe_rep_f<k>.bin (netdump_tool diff).
static bool compare_checked(sDevil4Main* main, int k)
{
    const std::vector<uint8_t>& forward = g_probe.checked[k % RollbackProbe::kHistory];
    static std::vector<uint8_t> replay;
    if (forward.empty() || !pack_checked(main, replay)) {
        return true;
    }
    if (replay == forward) {
        return true;
    }
    uint32_t first = 0;
    while (first < replay.size() && first < forward.size() && replay[first] == forward[first]) {
        ++first;
    }
    char line[160];
    snprintf(line, sizeof line, "  checked state differs at f%d: %u vs %u bytes, first at +%X", k,
        (unsigned)forward.size(), (unsigned)replay.size(), first);
    probe_log(line);
    if (g_probe.checked_differ++ < 3) {
        CreateDirectoryA("netdump", NULL);
        char name[MAX_PATH];
        sprintf_s(name, "netdump/probe_fwd_f%d.bin", k);
        if (FILE* out = std::fopen(name, "wb")) {
            std::fwrite(forward.data(), 1, forward.size(), out);
            std::fclose(out);
        }
        sprintf_s(name, "netdump/probe_rep_f%d.bin", k);
        if (FILE* out = std::fopen(name, "wb")) {
            std::fwrite(replay.data(), 1, replay.size(), out);
            std::fclose(out);
        }
    }
    return false;
}

// Diagnostics: what turns a player's stick into a direction (uPlayer::setCameraAngle 0x7ABCB0 -> +0x16F8 / +0x16F4,
// setInterfaceAnlgInfo 0x7A9550 -> mAnlg), and the camera eye / target it reads.
static void trace_players(float out[2][10])
{
    std::memset(out, 0, sizeof(float) * 20);
    for (int i = 0; i < 2; ++i) {
        const uintptr_t pl = player_info::player(i);
        if (!pl) {
            continue;
        }
        out[i][0] = *(const float*)(pl + 0x16F8);
        out[i][1] = *(const float*)(pl + 0x16F4);
        out[i][2] = *(const float*)(pl + 0x141C);   // mAnlg.pow (as bits, a float view)
        out[i][3] = *(const float*)(pl + 0x1420);   // mAnlg.radian
        Coop* coop = get_coop_mod();
        const uintptr_t cam = i == 0 || !(coop && coop->mod_enabled) ? *(const uintptr_t*)(*(const uintptr_t*)0x00E558B8 + 0xD0)
                                                                      : (uintptr_t)coop::players[i].cam;
        if (cam) {
            for (int k = 0; k < 3; ++k) {
                out[i][4 + k] = *(const float*)(cam + 0x30 + 4 * k);
                out[i][7 + k] = *(const float*)(cam + 0x50 + 4 * k);
            }
        }
    }
}

// Diagnostics: player 2's camera unit, whole (heap block).
static void copy_cam_block(std::vector<uint8_t>& out)
{
    out.clear();
    Coop* coop = get_coop_mod();
    const uintptr_t cam = coop && coop->mod_enabled ? (uintptr_t)coop::players[1].cam : 0;
    uintptr_t block = 0;
    uint32_t bytes = 0;
    if (!cam || !heap_snapshot::locate(cam, block, bytes) || block != cam || bytes > 0x10000) {
        return;
    }
    out.assign((const uint8_t*)cam, (const uint8_t*)cam + bytes);
}

static void compare_cam_block(int k)
{
    static std::vector<uint8_t> now;
    copy_cam_block(now);
    const std::vector<uint8_t>& was = g_probe.cam_block[k % RollbackProbe::kHistory];
    static int reports = 0;
    if (now.empty() || now.size() != was.size() || now == was || reports >= 12) {
        return;
    }
    ++reports;
    char line[400];
    int at = snprintf(line, sizeof line, "  camera 2 block differs at f%d:", k);
    int runs = 0;
    for (uint32_t o = 0; o + 4 <= now.size() && runs < 24; o += 4) {
        if (std::memcmp(&now[o], &was[o], 4) != 0) {
            float a, b;
            std::memcpy(&a, &was[o], 4);
            std::memcpy(&b, &now[o], 4);
            at += snprintf(line + at, sizeof line - at, " +%X(%g/%g)", o, a, b);
            ++runs;
            if (at > (int)sizeof line - 40) {
                break;
            }
        }
    }
    probe_log(line);
}

static void compare_trace(int k)
{
    float now[2][10];
    trace_players(now);
    const float (&was)[2][10] = g_probe.trace[k % RollbackProbe::kHistory];
    static const char* names[10] = { "cam angle", "angle", "pow", "radian", "eye x", "eye y", "eye z", "at x", "at y", "at z" };
    static int lines = 0;
    for (int i = 0; i < 2; ++i) {
        for (int f = 0; f < 10; ++f) {
            if (std::memcmp(&now[i][f], &was[i][f], 4) != 0 && lines < 400) {
                ++lines;
                char line[160];
                snprintf(line, sizeof line, "  trace f%d player %d %s: forward %.6f, replay %.6f", k, i + 1, names[f],
                    was[i][f], now[i][f]);
                probe_log(line);
            }
        }
    }
}

static void probe_save(sDevil4Main* main, int frame, bool keep_timing)
{
    heap_snapshot::save(frame);
    if (keep_timing) {
        netplay_snapshot::capture_timing_state(main, g_sctx.g_simTimeUs,
            g_probe.timing[frame % RollbackProbe::kHistory]);
    }
    pin_tick_rng_from_live();
}

// Simulates the frames after `base` up to the newest save again, from the state saved at `base`.
static bool probe_rerun(sDevil4Main* main, int base, bool made_up)
{
    RollbackProbe& p = g_probe;
    char line[96];
    snprintf(line, sizeof line, "  rerun f%d from f%d%s", p.frame, base, made_up ? " made-up" : "");
    probe_log(line);
    if (!heap_snapshot::load(base)) {
        return false;
    }
    sound_after_rewind();
    netplay_snapshot::apply_timing_state(main, p.timing[base % RollbackProbe::kHistory], g_sctx.g_simTimeUs);
    pin_tick_rng_from_live();
    run_tail_simulation(main);   // the save sits between a frame's tick and its tail
    uint32_t previous_on = p.inputs[base % RollbackProbe::kHistory].mBtnOn;
    g_probe_phase = made_up ? "made-up" : "real";
    bool reported = false;   // one line per replay: the first frame whose checksummed state differs
    for (int k = base + 1; k < p.frame; ++k) {
        g_probe_tick = k;
        const NetplayInputInfo& real = p.inputs[k % RollbackProbe::kHistory];
        const NetplayInputInfo input = made_up ? probe_made_up_input(real, previous_on) : real;
        apply_recorded_input((const unsigned char*)&input);
        if (g_probe_coop) {
            static uint32_t previous_on2 = 0;
            const NetplayInputInfo& real2 = p.inputs2[k % RollbackProbe::kHistory];
            const NetplayInputInfo input2 = made_up ? probe_made_up_input(real2, previous_on2) : real2;
            apply_recorded_input_control(input2, 1);
        }
        g_probe_replay = true;
        run_simulation_tick(main, nullptr, g_sctx);
        g_probe_replay = false;
        if (!made_up && !reported) {
            compare_trace(k);
            compare_cam_block(k);
            reported = !compare_checked(main, k);
        }
        probe_save(main, k, !made_up);
        run_tail_simulation(main);
    }
    g_probe_phase = "forward";
    return true;
}

static void probe_run(sDevil4Main* main, int base)
{
    RollbackProbe& p = g_probe;
    if (!heap_snapshot::verify_begin(base)) {
        ++p.skipped;
        return;
    }
    g_sctx.g_resimulating = true;
    const bool ok = (!p.made_up_first || probe_rerun(main, base, true)) && probe_rerun(main, base, false);
    g_sctx.g_resimulating = false;
    if (!ok) {
        // the state is whatever the failed step left: stop probing rather than build on it
        char line[160];
        snprintf(line, sizeof line, "probe f%d: load failed (%s), probe stopped", p.frame, heap_snapshot::stats().last_error);
        probe_log(line);
        p.enabled = false;
        ++p.skipped;
        return;
    }
    p.replay_next = true;
    heap_snapshot::VerifyResult result;
    if (!heap_snapshot::verify_end(result)) {
        ++p.skipped;
        return;
    }
    ++p.runs;
    p.last = result;
    if (result.pages_differing) {
        ++p.failures;
    }
    static const bool quiet = std::getenv("DMC4_PROBE_VARY") != nullptr;   // many probes: the checked-state lines only
    if (!quiet && (result.pages_differing || result.data_bytes_differing)) {
        char line[256];
        snprintf(line, sizeof line, "probe f%d depth %d%s: %u of %u pages differ, .data (not restored) %u bytes",
            p.frame, p.depth, p.made_up_first ? " made-up first" : "", result.pages_differing,
            result.pages_checked, result.data_bytes_differing);
        probe_log(line);
        const local_world::Stats lw = local_world::stats();
        snprintf(line, sizeof line, "  local world %s: %u units, %u bytes, %u frees rerouted, %u outside allocations (first from %08X)",
            lw.ready ? "ready" : "not made", lw.units, lw.used, lw.frees_rerouted, lw.outside_allocs, (unsigned)lw.outside_from);
        probe_log(line);
        snprintf(line, sizeof line, "  local world: %u allocations routed from presentation code", lw.routed_allocs);
        probe_log(line);
        local_world::Site sites[12];
        const int n = local_world::sites(sites, 12);
        for (int i = 0; i < n; ++i) {
            snprintf(line, sizeof line, "  temp heap allocation from %08X: %u times, %u bytes", (unsigned)sites[i].from,
                sites[i].count, sites[i].bytes);
            probe_log(line);
        }
        for (int i = 0; i < result.mismatch_count; ++i) {
            const heap_snapshot::Mismatch& m = result.mismatches[i];
            snprintf(line, sizeof line, "  %08X %s: %u bytes, block %08X +%X of %X, first word %08X, referrer %08X (%08X) +%X",
                (unsigned)m.address, m.where, m.bytes, (unsigned)m.block, (unsigned)(m.block ? m.address - m.block : 0),
                m.block_bytes, m.first_word, (unsigned)m.referrer, m.referrer_word, m.referrer_offset);
            probe_log(line);
        }
    }
}

// Diagnostics: units of the local heap on a synced line (a rewind can't put their links back), with the stack that made them.
static void report_stray_local_units()
{
    static std::unordered_set<uintptr_t> seen;
    sUnit* su = (sUnit*)*(uintptr_t*)0x00E552CC;
    if (!su || !local_world::enabled()) {
        return;
    }
    static const uintptr_t dll = (uintptr_t)GetModuleHandleA("dinput8.dll");
    for (int line = 0; line < 32; ++line) {
        if (line == local_world::kLine) {
            continue;
        }
        int guard = 0;
        for (uintptr_t u = (uintptr_t)su->mMoveLine[line].mTop; u && guard < 100000; u = *(uintptr_t*)(u + 8), ++guard) {
            if (!local_world::contains(u) || !seen.insert(u).second) {
                continue;
            }
            void* stack[12];
            const int n = local_world::origin(u, stack, 12);
            char text[400];
            int at = snprintf(text, sizeof text, "local unit %08X (vtable %08X) on synced line %d, made by", (unsigned)u,
                *(const uint32_t*)u, line);
            for (int k = 0; k < n && at < (int)sizeof text - 16; ++k) {
                const uintptr_t a = (uintptr_t)stack[k];
                at += snprintf(text + at, sizeof text - at, a >= dll && a < dll + 0x1500000 ? " d+%X" : " %08X",
                    (unsigned)(a >= dll && a < dll + 0x1500000 ? a - dll : a));
            }
            probe_log(text);
        }
    }
}

static void probe_frame(sDevil4Main* main)
{
    RollbackProbe& p = g_probe;
    // test switch DMC4_PROBE_COOP: a second (idle) Coop player, armed as netplay arms it
    static bool coop_armed = false;
    if (!coop_armed && std::getenv("DMC4_PROBE_COOP")) {
        coop_armed = true;
        g_probe_coop = true;
        if (Coop* coop = get_coop_mod()) {
            coop->player_num = 2;
            if (!coop->mod_enabled) {
                coop->mod_enabled = true;
                coop->toggle(true);
            }
        }
    }
    // only inside a loaded room with a player, after it settled: loading can't be rewound (a rewind into the mission's
    // load left a black screen), so the snapshot starts again in every room
    sArea* area = devil4_sdk::get_sArea();
    const uintptr_t room = area ? (uintptr_t)area->aRoomPtr : 0;
    if (!room || player_info::count() == 0 || room != p.room) {
        probe_stop();
        p.room = room;
        p.settled = 0;
    }
    static const int settle = std::getenv("DMC4_PROBE_SETTLE") ? std::atoi(std::getenv("DMC4_PROBE_SETTLE")) : 120;   // test switch
    if (!p.running && (!room || player_info::count() == 0 || ++p.settled < settle)) {
        clear_tick_rng_pin();
        run_simulation_tick(main, nullptr, g_sctx);
        return;
    }
    if (!p.running) {
        clear_tick_rng_pin();
        if (!heap_snapshot::begin(p.depth + 4)) {
            p.enabled = false;
            spdlog::screen(fmt::format("rollback probe: {}", heap_snapshot::stats().last_error), 0xFF4C4CFF);
            run_simulation_tick(main, nullptr, g_sctx);
            return;
        }
        heap_snapshot::set_track_data(true);
        install_fault_logger();
        {
            char line[120];
            const uintptr_t unit = heap_snapshot::region_base("Unit");
            snprintf(line, sizeof line, "Unit heap at %08X", (unsigned)unit);
            probe_log(line);
            if (const char* watch = std::getenv("DMC4_WATCH")) {
                g_watch_address = unit + std::strtoul(watch, nullptr, 16);
                RaiseException(kArmWatch, 0, 0, nullptr);
                snprintf(line, sizeof line, "watching writes to %08X", (unsigned)g_watch_address);
                probe_log(line);
            }
        }
        local_world::count_sites(0x00C307AC, true);   // what allocates from the Temp heap during play
        p.running = true;
        p.frame = 0;
        p.last_probe = 0;
        p.replay_next = false;
    }
    {
        // diagnostics (DMC4_TRACE_RES): every write of the traced resource's count, on the game thread
        static uintptr_t armed = 0;
        const uintptr_t traced = heap_snapshot::traced_resource();
        if (traced && traced != armed && !std::getenv("DMC4_WATCH")) {
            armed = traced;
            g_watch_address = traced + 0x44;
            RaiseException(kArmWatch, 0, 0, nullptr);
            char line[80];
            snprintf(line, sizeof line, "watching the count of %08X", (unsigned)traced);
            probe_log(line);
        }
    }
    NetplayInputInfo live{};
    fill_live_input(live, game_pad());
    // test switch: random input on the forward ticks and no rewinds (does the input alone crash the game?)
    static const bool random_live = std::getenv("DMC4_RANDOM_LIVE") != nullptr;
    static uint32_t random_previous = 0;
    if (random_live) {
        live = probe_made_up_input(live, random_previous);
    }
    // test switch DMC4_PROBE_VARY: rewinds as online makes them, 1-8 frames deep every 1-4 frames
    static const bool vary = std::getenv("DMC4_PROBE_VARY") != nullptr;
    static int vary_depth = 8, vary_interval = 1;
    const int depth = vary ? vary_depth : p.depth;
    const int base = p.frame - 1 - depth;
    if (!random_live && p.frame - p.last_probe >= (vary ? vary_interval : p.interval) && base >= 0 &&
        heap_snapshot::has_frame(base)) {
        if (vary) {
            p.rng ^= p.rng << 13;
            p.rng ^= p.rng >> 17;
            p.rng ^= p.rng << 5;
            vary_depth = 1 + (int)(p.rng % 8);
            vary_interval = 1 + (int)((p.rng >> 8) % 4);
        }
        p.last_probe = p.frame;
        probe_run(main, base);
    }
    apply_recorded_input((const unsigned char*)&live);
    NetplayInputInfo live2{};
    if (g_probe_coop) {
        random_online_input(live2, 1);   // player 2 plays random input, as the peer does online
        apply_recorded_input_control(live2, 1);
    }
    g_probe_tick = p.frame;
    g_probe_replay = p.replay_next;
    p.replay_next = false;
    run_simulation_tick(main, nullptr, g_sctx);
    g_probe_replay = false;
    report_stray_local_units();
    pack_checked(main, p.checked[p.frame % RollbackProbe::kHistory]);
    trace_players(p.trace[p.frame % RollbackProbe::kHistory]);
    copy_cam_block(p.cam_block[p.frame % RollbackProbe::kHistory]);
    p.inputs[p.frame % RollbackProbe::kHistory] = live;
    p.inputs2[p.frame % RollbackProbe::kHistory] = live2;
    probe_save(main, p.frame, true);
    ++p.frame;
}

static bool fixed_timing_wanted()
{
    return g_netplay != nullptr || g_probe.enabled;
}

static void draw_rollback_ui()
{
    if (!ImGui::CollapsingHeader("Rollback state")) {
        return;
    }
    const bool session = g_netplay && g_netplay->get_session();
    ImGui::BeginDisabled(session);
    ImGui::Checkbox("Restore from heap pages", &g_heap_restore);
    ImGui::EndDisabled();
    ImGui::SameLine();
    gui::help_marker("On: a rollback puts back the game's own memory (its heaps, page by page), so everything the "
        "mispredicted frames did is undone. Off: only the fields the battle snapshot lists are restored. "
        "Takes effect at the next session.");
    ImGui::BeginDisabled(session || g_probe.running);
    if (ImGui::Checkbox("Also the Resource heap", &g_heap_resource)) {
        heap_snapshot::set_optional_heaps(g_heap_resource, g_heap_sound);
    }
    ImGui::SameLine();
    gui::help_marker("Rolls back the heap loaded files live in as well. The loader thread uses it too.");
    if (ImGui::Checkbox("Also the Sound heap", &g_heap_sound)) {
        heap_snapshot::set_optional_heaps(g_heap_resource, g_heap_sound);
    }
    ImGui::SameLine();
    gui::help_marker("Rolls back the sound system's heap as well. The sound thread uses it too.");
    ImGui::EndDisabled();

    const heap_snapshot::Stats& s = heap_snapshot::stats();
    if (!s.installed || s.heap_count == 0) {
        ImGui::TextDisabled("heap snapshot unavailable: %s", s.last_error);
    }
    if (ImGui::Checkbox("Rollback probe", &g_probe.enabled) && !g_probe.enabled) {
        probe_stop();
    }
    ImGui::SameLine();
    gui::help_marker("Without a session: every few frames the last frames are simulated again, first with "
        "random input, then with the real input, and the game's memory is compared with the first run. "
        "Mismatches show here and go to netplay_probe.log. Play normally while it runs.");
    ImGui::BeginDisabled(g_probe.running);
    ImGui::SliderInt("Probe depth", &g_probe.depth, 1, 30);
    ImGui::SameLine();
    gui::help_marker("Frames rewound by each probe.");
    ImGui::EndDisabled();
    ImGui::SliderInt("Probe interval", &g_probe.interval, 1, 300);
    ImGui::SameLine();
    gui::help_marker("Frames between probes.");
    ImGui::Checkbox("Random input first", &g_probe.made_up_first);
    ImGui::SameLine();
    gui::help_marker("Off: the frames are only simulated again with the input they had (what the stress test does).");
    ImGui::Text("probes %u, failed %u, skipped %u", g_probe.runs, g_probe.failures, g_probe.skipped);
    const heap_snapshot::VerifyResult& r = g_probe.last;
    ImGui::Text("last: %u of %u pages differ, .data (not restored) %u bytes", r.pages_differing, r.pages_checked,
        r.data_bytes_differing);
    for (int i = 0; i < r.mismatch_count && i < 8; ++i) {
        ImGui::Text("  %08X %s: %u bytes", (unsigned)r.mismatches[i].address, r.mismatches[i].where,
            r.mismatches[i].bytes);
    }
    if (s.active) {
        ImGui::Text("frames %d..%d, save %.2f ms (max %.2f), load %.2f ms (max %.2f, %u pages)", s.oldest_frame,
            s.newest_frame, s.last_save_ms, s.max_save_ms, s.last_load_ms, s.max_load_ms, s.last_load_pages);
        ImGui::Text("pool %.1f MiB, %u exclusions, %u root allocations while active", s.pool_bytes / 1048576.0,
            s.exclusions, s.direct_allocs);
    }
    for (int i = 0; i < s.heap_count; ++i) {
        const heap_snapshot::HeapStats& h = s.heaps[i];
        ImGui::Text("%-9s %08X %6.1f MiB %s shadow %u, dirty %u (max %u)", h.name, (unsigned)h.base,
            h.size / 1048576.0, h.included ? "in " : "out", h.shadow_pages, h.last_dirty, h.max_dirty);
    }
    const local_world::Stats lw = local_world::stats();
    ImGui::Text("local world %s: %u units, %.1f of %.0f MiB, %u ticks, %u frees rerouted, %u outside allocations (first from %08X)",
        local_world::enabled() ? "on" : "off", lw.units, lw.used / 1048576.0, lw.size / 1048576.0, lw.ticks, lw.frees_rerouted,
        lw.outside_allocs, (unsigned)lw.outside_from);
    ImGui::SameLine();
    gui::help_marker("Units only this machine sees (ModelExtension's chains and attachments) live in their own heap and line, "
        "kept out of rollback. Outside allocations are ones local code made from a synced heap anyway: they should stay 0.");
    ImGui::Checkbox("Allocator overlay", &g_show_allocator_statistics);
    ImGui::SameLine();
    gui::help_marker("Draws the game's heaps' use over the game.");
}
#pragma endregion

static void __stdcall devil4_main_move_loop(sDevil4Main* main) {
    // Every presented frame opens a page-observation interval; the journal's loop clock
    // only advances here so a rollback burst stays inside one interval.
    netplay_test::page_loop_begin();

    // the local world is on while a netplay mode or the rollback probe is (sdk/LocalWorld.hpp)
    if (local_world::enabled() != fixed_timing_wanted()) {
        static bool routed = false;
        if (!routed) {
            // the GPU particles' own per-frame allocations (their prelude and tail calls, 0xAA0000..0xAB0000)
            if (!GetEnvironmentVariableA("DMC4_NO_ROUTE", nullptr, 0)) {
                local_world::route_code(0x00AA0000, 0x00AB0000);
            }
            routed = true;
        }
        local_world::set_enabled(fixed_timing_wanted());
    }

    // while netplay or the probe runs the ticks, the mods' per-tick work runs at each tick's start (frame_hooks::TICK_START)
    frame_hooks::set_ticks_driven(fixed_timing_wanted());

    // re-armed below by whichever path actually ticks forward this presented frame
    g_native_tail_due = false;
    if (g_netplay && g_netplay->get_session()) {
        g_netplay->before_move(main);
        g_netplay->move(main);
        g_netplay->after_move(main);
    } else {
        // No session: every presented frame is a real tick of the plain (vanilla) simulation,
        // and the RNG pin must not survive into it - a stale pin would rewind the stream here.
        g_native_tail_due = true;
        if (g_probe.enabled) {
            probe_frame(main);
        } else {
            probe_stop();
            clear_tick_rng_pin();
            run_simulation_tick(main, nullptr, g_sctx);
        }
    }

    // main->mpCollision->sync(); // actually a nullsub :shrug:
}


// TODO(): unhook on freeing OnlineComponent
static std::unique_ptr<FunctionHook> g_sDevil4Main_update_loop_hook;
static const uintptr_t g_sDevil4Main_loop_jmp_back = 0x008AF04C;
// The first instruction after the tail block (0x8AF04C..0x8AF13B in the IDB): the save-error
// check that closes sDevil4Main::move. A presented frame without a forward tick jumps here
// after running only the presentation systems, so tail-only state stays where its tick left it
// (see g_native_tail_due).
static const uintptr_t g_sDevil4Main_tail_post_jmp_back = 0x008AF13D;
naked void detour_sDevil4Main_move_loop() {
    _asm {
        pushad
        push ebp
        call devil4_main_move_loop
        popad
        cmp byte ptr [g_native_tail_due], 0
        jne run_native_tail

        // presentation-only tail, in sDevil4Main::move order (0x8AF0E1..0x8AF130 in the IDB):
        // mpRender's per-frame move, its vertex-buffer unlock, the present and the focus check.
        // ebp = main and ebx = 1 survive (popad, then callee-saved through the calls).
        mov ecx, [ebp+10348h]
        mov edx, [ecx]
        mov eax, [edx+18h]
        call eax
        mov esi, [ebp+10378h]
        mov eax, 0xAA7F40
        call eax
        mov esi, [ebp+10348h]
        mov eax, 0x8F33B0
        call eax
        mov eax, 0x8AFD60
        call eax
        jmp DWORD PTR [g_sDevil4Main_tail_post_jmp_back]
    run_native_tail:
        jmp DWORD PTR [g_sDevil4Main_loop_jmp_back]
    }
}
#pragma endregion

void CoopOnline::on_gui_frame(int display) {
        help_marker(_("CoopOnlineHelpMarker"));

        const char* mode_names[] = { "None", "Online (gekkonet)", "Stress test (rollback verify)" };
        int mode = (int)g_mode;
        if (ImGui::Combo("Mode", &mode, mode_names, 3)) {
            select_mode((NetplayMode)mode);
        }
        draw_rollback_ui();
        if (g_netplay) {
            g_netplay->draw_ui();
        }
}

// void CoopOnline::on_game_pause(bool toggle) {}
// bool CoopOnline::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}
std::optional<std::string> install_hooks(CoopOnline* mod) {

    if (!mod->install_hook_absolute(0x008AEFD5, g_sDevil4Main_update_loop_hook, &detour_sDevil4Main_move_loop, NULL)) {
        spdlog::error("Failed to hook sDevil4Main::update loop\n");
        return "Failed to init online coop";
    }

    if (!mod->install_hook_absolute(0x0090D690, g_smain_move_timing_hook, &sMain_move_fixed, NULL)) {
        spdlog::error("Failed to hook sMain::move\n");
        return "Failed to hook sMain::move";
    }

    return std::nullopt;
}

// diagnostics (DMC4_SHAKE_LOG): every camera shake request (uDevilCamera::vibReq 0x531B60, its edi form 0x531A80,
// vib2Req 0x531AF0), with the tick and who asked, in the probe / online log
static std::unique_ptr<FunctionHook> g_shake_hooks[6];
static uintptr_t g_shake_original[6];
// a unit as both machines name it: its move line and place there (0xFFFFFFFF = not on a line)
static uint32_t unit_tag(uintptr_t unit)
{
    sUnit* su = (sUnit*)*(uintptr_t*)0x00E552CC;
    if (!su || !unit) {
        return 0xFFFFFFFFu;
    }
    for (int line = 0; line < 32; ++line) {
        int index = 0;
        for (uintptr_t u = (uintptr_t)su->mMoveLine[line].mTop; u && index < 4096; u = *(uintptr_t*)(u + 8), ++index) {
            if (u == unit) {
                return (uint32_t)(line << 16 | index);
            }
        }
    }
    return 0xFFFFFFFFu;
}
// uDamage::calc 0x51BE10 (esi = the uDamage, args: the attacking and the hit collision groups, +0x98 manager, +0xD8 the
// actor it reports for)
static void __stdcall log_hit(uintptr_t damage, uintptr_t attack_group, uintptr_t hit_group)
{
    const auto actor = [](uintptr_t group) -> uintptr_t {
        const uintptr_t mgr = group ? *(const uintptr_t*)(group + 0x98) : 0;
        return mgr ? *(const uintptr_t*)(mgr + 0xD8) : 0;
    };
    const uintptr_t a = actor(attack_group), v = actor(hit_group);
    const uint32_t ta = unit_tag(a), tv = unit_tag(v);
    char text[240];
    snprintf(text, sizeof text, "hit f%d%s pass %u att L%d#%d (%08X) at %.1f %.1f %.1f vic L%d#%d (%08X) at %.1f %.1f %.1f",
        g_tick_frame, g_sctx.g_resimulating ? " (again)" : "", g_tick_serial, (int)(ta >> 16), (int)(ta & 0xFFFF),
        a ? *(const uint32_t*)a : 0u, a ? *(const float*)(a + 0x30) : 0.0f, a ? *(const float*)(a + 0x34) : 0.0f,
        a ? *(const float*)(a + 0x38) : 0.0f, (int)(tv >> 16), (int)(tv & 0xFFFF), v ? *(const uint32_t*)v : 0u,
        v ? *(const float*)(v + 0x30) : 0.0f, v ? *(const float*)(v + 0x34) : 0.0f, v ? *(const float*)(v + 0x38) : 0.0f);
    (void)damage;
    probe_log(text);
}
static void __stdcall log_style(uint32_t from, uint32_t type, uint32_t a, uint32_t b, uint32_t c)
{
    float fa, fb, fc;
    std::memcpy(&fa, &a, 4);
    std::memcpy(&fb, &b, 4);
    std::memcpy(&fc, &c, 4);
    void* frames[16];
    const USHORT n = RtlCaptureStackBackTrace(1, 16, frames, nullptr);
    static const uintptr_t dll = (uintptr_t)GetModuleHandleA("dinput8.dll");
    char text[400];
    uintptr_t block = 0;
    uint32_t bytes = 0;
    const char* heap = heap_snapshot::locate(type, block, bytes);
    int at = snprintf(text, sizeof text, "style f%d%s pass %u from %08X type %u (%s +%X of %X, %08X) %g %g %g:", g_tick_frame,
        g_sctx.g_resimulating ? " (again)" : "", g_tick_serial, from, type, heap ? heap : "-",
        (unsigned)(block ? type - block : 0), bytes, block ? *(const uint32_t*)block : 0u, fa, fb, fc);
    for (USHORT k = 0; k < n && at < (int)sizeof text - 16; ++k) {
        const uintptr_t x = (uintptr_t)frames[k];
        at += snprintf(text + at, sizeof text - at, x >= dll && x < dll + 0x1500000 ? " d+%X" : " %08X",
            (unsigned)(x >= dll && x < dll + 0x1500000 ? x - dll : x));
    }
    probe_log(text);
}
static void __stdcall log_shake(int which, uintptr_t camera)
{
    void* frames[16];
    const USHORT n = RtlCaptureStackBackTrace(1, 16, frames, nullptr);
    static const uintptr_t dll = (uintptr_t)GetModuleHandleA("dinput8.dll");
    char text[400];
    int at = snprintf(text, sizeof text, "shake %d f%d%s pass %u camera %08X:", which, g_tick_frame,
        g_sctx.g_resimulating ? " (again)" : "", g_tick_serial, (unsigned)camera);
    for (USHORT k = 0; k < n && at < (int)sizeof text - 16; ++k) {
        const uintptr_t a = (uintptr_t)frames[k];
        at += snprintf(text + at, sizeof text - at, a >= dll && a < dll + 0x1500000 ? " d+%X" : " %08X",
            (unsigned)(a >= dll && a < dll + 0x1500000 ? a - dll : a));
    }
    probe_log(text);
}
naked void shake_detour0()
{
    __asm {
        pushad
        push dword ptr [esp + 36]   // vibReq(camera, ...): the camera
        push 0
        call log_shake
        popad
        jmp dword ptr [g_shake_original]
    }
}
naked void shake_detour1()
{
    __asm {
        pushad
        push edi
        push 1
        call log_shake
        popad
        jmp dword ptr [g_shake_original + 4]
    }
}
// sStylishCount::addComboGauge 0x4A5DC0 and addComboGauge2 0x4A5FE0 (the Snatch, the Buster, bonus points; eax = the
// instance; stack: return, key, ...): the style repetition guard
// (30 entries from +0x24: status, key, timer; setRepetitionGuard 0x4A6200) compares keys by value. A key is an .atk entry
// (the Resource heap, the same on both machines), a small number, or attack data inside a unit (a shell's own): that one
// is an address, and a shell made where a dead one was counts as the same attack. The machines' heaps are laid out
// apart (each spent its own time in the room before the session), so that reuse happened on one machine only and the
// style gains went apart. Online, a key inside the Unit heap becomes its unit's class and its place in it: the same on
// both machines, and what the reuse made it most of the time.
static std::unique_ptr<FunctionHook> g_combo_gauge_hook, g_combo_gauge2_hook;
static uintptr_t g_combo_gauge_original = 0, g_combo_gauge2_original = 0;
static bool fixed_timing_wanted();
static void __stdcall combo_gauge_entry(uint32_t* stack, int which)
{
    static const bool log = std::getenv("DMC4_SHAKE_LOG") != nullptr;
    if (log) {
        log_style(stack[0], stack[1], which == 0 ? stack[3] : stack[2], which == 0 ? stack[4] : stack[3],
            which == 0 ? stack[5] : stack[4]);
        // the game's return addresses on the stack (its frames have no frame pointers for a walk)
        char text[300];
        int at = snprintf(text, sizeof text, "style stack f%d pass %u:", g_tick_frame, g_tick_serial);
        int found = 0;
        for (int k = 0; k < 400 && found < 14 && at < (int)sizeof text - 12; ++k) {
            const uint32_t v = stack[k];
            if (v >= 0x00401000u && v < 0x00B85000u) {
                at += snprintf(text + at, sizeof text - at, " %08X", v);
                ++found;
            }
        }
        probe_log(text);
    }
    if (!fixed_timing_wanted() || !heap_snapshot::active()) {
        return;
    }
    uintptr_t block = 0;
    uint32_t bytes = 0;
    const char* heap = heap_snapshot::locate(stack[1], block, bytes);
    if (!heap || !block || std::strcmp(heap, "Unit") != 0) {
        return;
    }
    const uint32_t vtable = *(const uint32_t*)block;
    const uint32_t kind = vtable >= 0x00400000u && vtable < 0x01000000u ? vtable : bytes;   // a mod's unit: its size
    stack[1] = ((kind * 2654435761u) ^ ((stack[1] - (uint32_t)block) << 1)) | 1u;   // odd: never an address
}
naked void combo_gauge_detour()
{
    __asm {
        pushad
        push 0
        lea eax, [esp + 36]
        push eax
        call combo_gauge_entry
        popad
        jmp dword ptr [g_combo_gauge_original]
    }
}
naked void combo_gauge2_detour()
{
    __asm {
        pushad
        push 1
        lea eax, [esp + 36]
        push eax
        call combo_gauge_entry
        popad
        jmp dword ptr [g_combo_gauge2_original]
    }
}
// sUnit::addBottom 0x8DC540 (eax = sUnit, esi = the unit, stack: the line): every unit put on a move line
static void __stdcall log_spawn(uintptr_t unit, uint32_t line, const uint32_t* stack)
{
    if (line == (uint32_t)local_world::kLine || !unit) {
        return;
    }
    static const uintptr_t dll = (uintptr_t)GetModuleHandleA("dinput8.dll");
    char text[400];
    int at = snprintf(text, sizeof text, "spawn f%d%s pass %u line %u vtable %08X:", g_tick_frame,
        g_sctx.g_resimulating ? " (again)" : "", g_tick_serial, line, *(const uint32_t*)unit);
    // return addresses on the stack, the game's and this DLL's (the game's frames can't be walked)
    int found = 0;
    for (int k = 0; k < 300 && found < 12 && at < (int)sizeof text - 16; ++k) {
        const uint32_t x = stack[k];
        const bool game = x >= 0x00401000u && x < 0x00B85000u;
        const bool ours = x >= dll && x < dll + 0x1500000;
        if (game || ours) {
            at += snprintf(text + at, sizeof text - at, ours ? " d+%X" : " %08X", (unsigned)(ours ? x - dll : x));
            ++found;
        }
    }
    probe_log(text);
}
naked void spawn_detour()
{
    __asm {
        pushad
        lea eax, [esp + 32]         // the caller's stack
        push eax
        push dword ptr [esp + 40]   // the line
        push esi
        call log_spawn
        popad
        jmp dword ptr [g_shake_original + 12]
    }
}
// uEnemy::grabMessage 0x734EE0 (ecx = the enemy; stack: the grabbing collision group, the hit one)
static void __stdcall log_grab(uintptr_t enemy, uintptr_t group)
{
    const uint32_t tag = unit_tag(enemy);
    char text[200];
    snprintf(text, sizeof text, "grab f%d%s pass %u enemy L%d#%d (%08X) kind %d +EAC %d +14F0 %08X", g_tick_frame,
        g_sctx.g_resimulating ? " (again)" : "", g_tick_serial, (int)(tag >> 16), (int)(tag & 0xFFFF),
        enemy ? *(const uint32_t*)enemy : 0u, group ? *(const int*)(group + 180) : -1, enemy ? *(const int*)(enemy + 0xEAC) : -1,
        enemy ? *(const uint32_t*)(enemy + 0x14F0) : 0u);
    probe_log(text);
}
naked void grab_detour()
{
    __asm {
        pushad
        push dword ptr [esp + 36]   // the grabbing group
        push ecx
        call log_grab
        popad
        jmp dword ptr [g_shake_original + 20]
    }
}
naked void hit_detour()
{
    __asm {
        pushad
        push dword ptr [esp + 40]   // the hit group
        push dword ptr [esp + 40]   // the attacking group
        push esi
        call log_hit
        popad
        jmp dword ptr [g_shake_original + 16]
    }
}
naked void shake_detour2()
{
    __asm {
        pushad
        push edi
        push 2
        call log_shake
        popad
        jmp dword ptr [g_shake_original + 8]
    }
}

std::optional<std::string> CoopOnline::on_initialize() {
    capture_canonical_environment();
    if (std::getenv("DMC4_SHAKE_LOG")) {
        // + uDamage::calc (sStylishCount::addComboGauge logs through combo_gauge_entry)
        // + sUnit::addBottom, uEnemy::grabMessage
        const uintptr_t sites[6] = {0x00531B60, 0x00531A80, 0x00531AF0, 0x008DC540, 0x0051BE10, 0x00734EE0};
        const uintptr_t detours[6] = {(uintptr_t)&shake_detour0, (uintptr_t)&shake_detour1, (uintptr_t)&shake_detour2,
            (uintptr_t)&spawn_detour, (uintptr_t)&hit_detour, (uintptr_t)&grab_detour};
        for (int i = 0; i < 6; ++i) {
            if (!sites[i]) {
                continue;
            }
            g_shake_hooks[i] = std::make_unique<FunctionHook>(sites[i], detours[i]);
            if (g_shake_hooks[i]->create()) {
                g_shake_original[i] = g_shake_hooks[i]->get_original();
            }
        }
    }
    // the game's counter for hit IDs (sMediator::getCollisionUniqueID 0x494990) lives in .data, which a rewind leaves:
    // a re-simulated attack took the next ID instead of the one it had, and the machines' IDs went apart
    rollback::add_range("uCollisionMgr::mCurrentUniqueID", (void*)0x00E558D4, 4);
    g_combo_gauge_hook = std::make_unique<FunctionHook>(0x004A5DC0, (uintptr_t)&combo_gauge_detour);
    if (!g_combo_gauge_hook->create()) {
        return "Failed to hook sStylishCount::addComboGauge";
    }
    g_combo_gauge_original = g_combo_gauge_hook->get_original();
    g_combo_gauge2_hook = std::make_unique<FunctionHook>(0x004A5FE0, (uintptr_t)&combo_gauge2_detour);
    if (!g_combo_gauge2_hook->create()) {
        return "Failed to hook sStylishCount::addComboGauge2";
    }
    g_combo_gauge2_original = g_combo_gauge2_hook->get_original();
    g_request_se_hook = std::make_unique<FunctionHook>(0x00A47AA0, (uintptr_t)&request_se_detour);
    if (!g_request_se_hook->create()) {
        return "Failed to hook sDevil4Sound::requestSe";
    }
    if (auto res = install_hooks(this)) {
        return res;
    }
    spdlog::screen(fmt::format("netplay state_size: {} bytes", netplay_snapshot::state_bytes()));
    return std::nullopt;
}

void CoopOnline::on_frame(fmilliseconds& dt) {
    if (g_netplay) {
        g_netplay->on_frame();
    }
    spdlog::hehe_draw();
    spdlog::hehe_flush();
}

void CoopOnline::on_config_load(const utility::Config& cfg) {
    netplay_test::configure_from_command_line();

    // Component selection: -netplay for online play, -stress for the stress test; the
    // harness/demo arguments imply the stress component. Otherwise restore the saved mode.
    select_mode_from_args();
    if (!g_netplay) {
        int saved = cfg.get<int>("coop_online_mode").value_or(0);
        if (saved < 0 || saved > 2) {
            saved = 0;
        }
        select_mode((NetplayMode)saved);
    }

    g_stress_component.on_config_load(cfg);

    g_smain_prelude_enabled = utility::check_argument("-netplay-prelude");

    g_heap_restore = cfg.get<bool>("netplay_heap_restore").value_or(true);
    g_heap_resource = cfg.get<bool>("netplay_heap_resource").value_or(false);
    g_heap_sound = cfg.get<bool>("netplay_heap_sound").value_or(false);
    heap_snapshot::set_optional_heaps(g_heap_resource, g_heap_sound);
    g_probe.enabled = cfg.get<bool>("netplay_probe").value_or(false);
    g_probe.depth = std::clamp(cfg.get<int>("netplay_probe_depth").value_or(8), 1, 30);
    g_probe.interval = std::clamp(cfg.get<int>("netplay_probe_interval").value_or(30), 1, 300);
    g_probe.made_up_first = cfg.get<bool>("netplay_probe_random_first").value_or(true);
    if (!g_probe.enabled) {
        probe_stop();
    }

    if (g_mode == NetplayMode::Online) {
        g_online_component.arm_from_command_line();
    }
}

void CoopOnline::on_config_save(utility::Config& cfg) {
    cfg.set<int>("coop_online_mode", (int)g_mode);
    g_stress_component.on_config_save(cfg);
    cfg.set<bool>("netplay_heap_restore", g_heap_restore);
    cfg.set<bool>("netplay_heap_resource", g_heap_resource);
    cfg.set<bool>("netplay_heap_sound", g_heap_sound);
    cfg.set<bool>("netplay_probe", g_probe.enabled);
    cfg.set<int>("netplay_probe_depth", g_probe.depth);
    cfg.set<int>("netplay_probe_interval", g_probe.interval);
    cfg.set<bool>("netplay_probe_random_first", g_probe.made_up_first);
}

void CoopOnline::on_stage_start() {
    // Safety net: a stage can load before on_config_load ran, so re-check the arguments.
    if (!g_netplay) {
        select_mode_from_args();
    }
    if (!netplay_test::wait_for_recorder()) {
        g_stress_component.failed = true;
        netplay_test::page_failed();
        netplay_test::complete("harness_error", 0);
        return;
    }
    probe_stop();
    if (g_netplay) {
        static constexpr uint32_t kStageRngSeed = 0xB16B00B5;
        mt_random_init(kStageRngSeed, mt_random_global());
        g_netplay->on_stage_start();
    }
    // The stage seed is the pin, taken here and not inside start_session so no end_session in
    // between can clear it: the session's first save runs before any tick (g_tick_rng_fresh)
    // and records this seed, whatever the machines' differing connect waits drew before it.
}

void CoopOnline::on_stage_end() {
    probe_stop();
    if (g_netplay) {
        g_netplay->on_stage_end();
    }
    netplay_test::complete(g_stress_component.desyncs ? "desync" : "stage_end",
        g_stress_component.desyncs);
}