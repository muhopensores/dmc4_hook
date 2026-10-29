#include "FileTracker.hpp"
#include "../../sdk/Devil4.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace file_tracker {

bool mod_enabled   = false;
int  scan_interval = 10;

namespace {

constexpr uintptr_t kResourceInstance = 0x00E552D0;   // sDevil4Resource::mpInstance (an sResource*)
constexpr uintptr_t kCriticalSection  = 0x0004;       // sResource: the table's CRITICAL_SECTION
constexpr uintptr_t kTable            = 0x1038;       // sResource: cResource* mpTable[8192], 2048 buckets of 4
constexpr int       kSlots            = 8192;
constexpr uintptr_t kPath = 0x04, kRefs = 0x44, kAttr = 0x48, kState = 0x4C, kSize = 0x50;   // cResource
constexpr uintptr_t kArcChildren = 0x60, kArcCount = 0x64;                                  // rArchive: cResource*[], count
constexpr uintptr_t kMotionLists = 0xA38;     // uModel: rMotionList* mpMotionListArr[16], the motion banks
constexpr uintptr_t kCoatModel   = 0x20C4;    // uPlayer: mpCoatMdl (the coat model, its own banks)
constexpr size_t    kLogMax = 4000;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// cResource vtable: slot 4 getDTI (-> MtDTI, name at +4), slot 6 getExt; both thiscall without arguments
const char* class_of(uintptr_t rsrc) {
    const uintptr_t* vt = *reinterpret_cast<uintptr_t**>(rsrc);
    const uintptr_t dti = ((uintptr_t(__fastcall*)(uintptr_t, int))vt[4])(rsrc, 0);
    const char* name = dti ? at<const char*>(dti, 4) : nullptr;
    return name ? name : "";
}
const char* ext_of(uintptr_t rsrc) {
    const uintptr_t* vt = *reinterpret_cast<uintptr_t**>(rsrc);
    const char* ext = ((const char*(__fastcall*)(uintptr_t, int))vt[6])(rsrc, 0);
    return ext ? ext : "";
}

std::mutex g_lock;
std::vector<Entry> g_entries;             // the last scan
std::vector<Event> g_log;
std::unordered_map<uintptr_t, std::string> g_seen;   // rsrc -> path at the last scan (a reused pointer with another path counts as new)
uint64_t g_frame = 0;
std::chrono::steady_clock::time_point g_start = std::chrono::steady_clock::now();

double seconds_now() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - g_start).count(); }

void push_event(bool added, const Entry& e) {
    if (g_log.size() >= kLogMax) g_log.erase(g_log.begin(), g_log.begin() + kLogMax / 4);
    g_log.push_back(Event{ g_frame, seconds_now(), added, e });
}

std::string state_text(const Entry& e) {
    std::string s;
    if (e.state & 0x1) s += "loaded";
    if (e.state & 0x10) s += s.empty() ? "failed" : ", failed";
    if (!(e.state & 0x11)) s += "pending";
    if (e.attr & 0x800) s += ", pinned";
    if (e.attr & 0x40) s += ", async";
    return s;
}

} // namespace

void scan() {
    const uintptr_t res = at<uintptr_t>(kResourceInstance, 0);
    if (!res) return;
    ++g_frame;
    std::vector<Entry> now;
    now.reserve(g_entries.size() + 64);
    std::unordered_map<uintptr_t, size_t> index;

    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(res + kCriticalSection));
    for (int i = 0; i < kSlots; ++i) {
        const uintptr_t r = at<uintptr_t>(res, kTable + 4 * i);
        if (!r) continue;
        Entry e;
        e.rsrc  = r;
        const char* p = &at<char>(r, kPath);
        e.path.assign(p, strnlen(p, 64));
        e.ext   = ext_of(r);
        e.cls   = class_of(r);
        e.refs  = at<uint32_t>(r, kRefs);
        e.attr  = at<uint32_t>(r, kAttr);
        e.state = at<uint32_t>(r, kState);
        e.size  = at<uint32_t>(r, kSize);
        e.is_arc = e.cls == "rArchive";
        index[r] = now.size();
        now.push_back(std::move(e));
    }
    for (size_t a = 0; a < now.size(); ++a) {          // the arcs' entries: which arc a file came from, and what is in use
        if (!now[a].is_arc) continue;
        const uintptr_t list = at<uintptr_t>(now[a].rsrc, kArcChildren);
        const uint32_t  n    = at<uint32_t>(now[a].rsrc, kArcCount);
        if (!list || n > 8192) continue;
        now[a].children = (int)n;
        for (uint32_t k = 0; k < n; ++k) {
            const auto it = index.find(at<uintptr_t>(list, 4 * k));
            if (it == index.end()) continue;
            Entry& c = now[it->second];
            if (c.arc.empty()) c.arc = now[a].path;
            if (c.refs > 1) ++now[a].used;
        }
    }
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(res + kCriticalSection));

    std::lock_guard<std::mutex> lock(g_lock);
    std::unordered_map<uintptr_t, std::string> seen;
    seen.reserve(now.size());
    for (const Entry& e : now) {
        const auto it = g_seen.find(e.rsrc);
        if (it == g_seen.end() || it->second != e.path) push_event(true, e);
        seen[e.rsrc] = e.path;
    }
    for (const Entry& e : g_entries) {
        const auto it = seen.find(e.rsrc);
        if (it == seen.end() || it->second != e.path) push_event(false, e);
    }
    g_seen = std::move(seen);
    g_entries = std::move(now);
}

std::vector<Entry> entries() { std::lock_guard<std::mutex> lock(g_lock); return g_entries; }
std::vector<Event> events()  { std::lock_guard<std::mutex> lock(g_lock); return g_log; }
void clear_log()             { std::lock_guard<std::mutex> lock(g_lock); g_log.clear(); }

bool save_list(const char* file) {
    const std::vector<Entry> list = entries();
    FILE* f = nullptr;
    if (fopen_s(&f, file, "w") != 0 || !f) return false;
    fprintf(f, "path\text\tclass\tarc\trefs\tsize\tstate\n");
    for (const Entry& e : list)
        fprintf(f, "%s\t%s\t%s\t%s\t%u\t%u\t%s%s\n", e.path.c_str(), e.ext.c_str(), e.cls.c_str(), e.arc.c_str(), e.refs, e.size,
                state_text(e).c_str(), e.is_arc ? (" (" + std::to_string(e.children) + " entries, " + std::to_string(e.used) + " in use)").c_str() : "");
    fclose(f);
    return true;
}

bool save_log(const char* file) {
    const std::vector<Event> log = events();
    FILE* f = nullptr;
    if (fopen_s(&f, file, "w") != 0 || !f) return false;
    fprintf(f, "seconds\tscan\tchange\tpath\text\tclass\tarc\n");
    for (const Event& ev : log)
        fprintf(f, "%.2f\t%llu\t%s\t%s\t%s\t%s\t%s\n", ev.seconds, (unsigned long long)ev.frame, ev.added ? "+" : "-", ev.e.path.c_str(),
                ev.e.ext.c_str(), ev.e.cls.c_str(), ev.e.arc.c_str());
    fclose(f);
    return true;
}

} // namespace file_tracker

// ---------------------------------------------------------------------------------------------------------------
using namespace file_tracker;

namespace {
int  g_since_scan = 0;
bool g_only_used = false;        // hide arc entries nothing but their arc holds
bool g_only_files = true;        // hide the arcs themselves from the list (they get their own section)
bool g_log_filtered = true;      // the log shows only what the filter matches
uint64_t g_mark = 0;             // the log shows changes after this scan
ImGuiTextFilter g_filter;
char g_save_name[128] = "file_tracker.txt";

bool matches(const Entry& e) {
    return g_filter.PassFilter(e.path.c_str()) || g_filter.PassFilter(e.ext.c_str()) || g_filter.PassFilter(e.cls.c_str()) ||
           g_filter.PassFilter(e.arc.c_str());
}
void set_filter(const char* text) { strcpy_s(g_filter.InputBuf, text); g_filter.Build(); }
void copy_row(const char* path) { ImGui::SetClipboardText(path); }
} // namespace

std::optional<std::string> FileTracker::on_initialize() { return Mod::on_initialize(); }

void FileTracker::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (++g_since_scan < std::max(scan_interval, 1)) return;
    g_since_scan = 0;
    scan();
}

void FileTracker::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("File tracker"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Lists every file the game holds loaded right now (from the arcs and loose files alike), which .arc each came "
                  "from, and whether anything uses it, and logs what gets loaded and dropped as you play. For finding which "
                  "file holds a motion, effect or model you saw: start a cutscene, then look at what appeared. Reads only."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("FileTracker");
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));   // the tables

    ImGui::Spacing();
    ImGui::SliderInt(_("Scan every N frames"), &scan_interval, 1, 60);
    ImGui::SameLine();
    help_marker(_("How often the resource table is read. A file loaded and freed within one interval is missed by the log."));

    ImGui::Spacing();
    g_filter.Draw(_("Filter"), 260.0f);
    ImGui::SameLine();
    help_marker(_("Matches the path, type, class or arc; several words = any of them, -word excludes. The buttons set common types."));
    ImGui::SameLine(); if (ImGui::SmallButton("lmt")) set_filter("lmt");
    ImGui::SameLine(); if (ImGui::SmallButton("efl")) set_filter("efl");
    ImGui::SameLine(); if (ImGui::SmallButton("mod")) set_filter("mod");
    ImGui::SameLine(); if (ImGui::SmallButton("sdl")) set_filter("sdl");
    ImGui::SameLine(); if (ImGui::SmallButton(_("all"))) set_filter("");
    ImGui::Checkbox(_("Only files in use"), &g_only_used);
    ImGui::SameLine();
    help_marker(_("Hide arc entries nothing but their arc holds (a loaded arc keeps every file in it; the game references the ones it uses)."));
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::Checkbox(_("Hide the arcs"), &g_only_files);

    const std::vector<Entry> list = entries();
    int arcs = 0;
    for (const Entry& e : list) if (e.is_arc) ++arcs;
    ImGui::Spacing();
    ImGui::Text(_("%d resources, %d arcs | click a path to copy it"), (int)list.size(), arcs);
    ImGui::PushItemWidth(200.0f);
    ImGui::InputText("##save_name", g_save_name, sizeof(g_save_name));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button(_("Save list"))) save_list(g_save_name);
    ImGui::SameLine();
    if (ImGui::Button(_("Save log"))) save_log((std::string("log_") + g_save_name).c_str());
    ImGui::SameLine();
    help_marker(_("Tab-separated text files in the game folder: the list as it is now, and the whole change log."));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- the arcs ----
    if (!g_only_files && ImGui::CollapsingHeader(_("Loaded arcs"))) {
        if (ImGui::BeginTable("arcs", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(_("Arc"), ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn(_("Entries"));
            ImGui::TableSetupColumn(_("In use"));
            ImGui::TableSetupColumn(_("Refs"));
            ImGui::TableHeadersRow();
            for (const Entry& e : list) {
                if (!e.is_arc || !matches(e)) continue;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Selectable(e.path.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) copy_row(e.path.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%d", e.children);
                ImGui::TableNextColumn(); ImGui::Text("%d", e.used);
                ImGui::TableNextColumn(); ImGui::Text("%u", e.refs);
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    // ---- the files ----
    std::vector<const Entry*> shown;
    shown.reserve(list.size());
    for (const Entry& e : list) {
        if (e.is_arc && g_only_files) continue;
        if (g_only_used && !e.is_arc && !e.arc.empty() && e.refs <= 1) continue;
        if (!matches(e)) continue;
        shown.push_back(&e);
    }
    std::sort(shown.begin(), shown.end(), [](const Entry* a, const Entry* b) { return a->path < b->path; });
    if (ImGui::CollapsingHeader((std::string(_("Loaded now")) + " (" + std::to_string(shown.size()) + ")###loaded").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("files", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
                              ImVec2(0.0f, 360.0f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(_("Path"), ImGuiTableColumnFlags_WidthStretch, 4.0f);
            ImGui::TableSetupColumn(_("Class"), ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn(_("From arc"), ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn(_("Refs"));
            ImGui::TableSetupColumn(_("Size"));
            ImGui::TableSetupColumn(_("State"), ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin((int)shown.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                    const Entry& e = *shown[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    if (ImGui::Selectable((e.path + "." + e.ext).c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) copy_row(e.path.c_str());
                    ImGui::PopID();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(e.cls.c_str());
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(e.arc.empty() ? (e.is_arc ? "" : "(loose / own load)") : e.arc.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%u", e.refs);
                    ImGui::TableNextColumn(); ImGui::Text("%u", e.size);
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(state_text(e).c_str());
                }
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    // ---- the player's motion banks: not all of them are files (the motion registry's merged lists never enter the
    // resource table: path motion\registry_bankN, no file behind them) ----
    if (ImGui::CollapsingHeader(_("Player motion banks"))) {
        ImGui::SameLine();
        help_marker(_("The 16 motion banks of the player's body and coat models: a motion number is bank << 8 | block. A bank named "
                      "motion\\registry_bankN is the motion registry's merged list (several mods' files; the registry's own "
                      "readouts say which), not a file."));
        if (uPlayer* pl = devil4_sdk::get_local_player()) {
            const uintptr_t models[2] = { (uintptr_t)pl, at<uintptr_t>((uintptr_t)pl, kCoatModel) };
            if (ImGui::BeginTable("banks", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn(_("Bank"));
                ImGui::TableSetupColumn(_("Body"), ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn(_("Coat"), ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableHeadersRow();
                for (int bank = 0; bank < 16; ++bank) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%d", bank);
                    for (int m = 0; m < 2; ++m) {
                        ImGui::TableNextColumn();
                        const uintptr_t list = models[m] ? at<uintptr_t>(models[m], kMotionLists + 4 * bank) : 0;
                        if (!list) { ImGui::TextDisabled("-"); continue; }
                        const char* path = &at<char>(list, kPath);
                        const std::string label = std::string(path, strnlen(path, 64)) + " (refs " + std::to_string(at<uint32_t>(list, kRefs)) + ")";
                        ImGui::PushID(bank * 2 + m);
                        if (ImGui::Selectable(label.c_str())) copy_row(std::string(path, strnlen(path, 64)).c_str());
                        ImGui::PopID();
                    }
                }
                ImGui::EndTable();
            }
        } else ImGui::TextDisabled(_("no player"));
    }

    ImGui::Spacing();
    // ---- the log ----
    const std::vector<Event> log = events();
    if (ImGui::CollapsingHeader((std::string(_("Changes")) + " (" + std::to_string(log.size()) + ")###changes").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button(_("Mark"))) g_mark = log.empty() ? 0 : log.back().frame;
        ImGui::SameLine();
        help_marker(_("Shows only changes after this point: press it before starting the cutscene, then see what got loaded."));
        ImGui::SameLine();
        if (ImGui::Button(_("Show all"))) g_mark = 0;
        ImGui::SameLine();
        if (ImGui::Button(_("Clear log"))) { clear_log(); g_mark = 0; }
        ImGui::SameLine();
        ImGui::Checkbox(_("Filter applies"), &g_log_filtered);
        ImGui::Spacing();
        std::vector<const Event*> rows;
        rows.reserve(log.size());
        for (const Event& ev : log) {
            if (ev.frame <= g_mark) continue;
            if (g_log_filtered && !matches(ev.e)) continue;
            rows.push_back(&ev);
        }
        if (ImGui::BeginTable("log", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
                              ImVec2(0.0f, 280.0f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(_("Time"));
            ImGui::TableSetupColumn("+/-");
            ImGui::TableSetupColumn(_("Path"), ImGuiTableColumnFlags_WidthStretch, 4.0f);
            ImGui::TableSetupColumn(_("Class"), ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn(_("From arc"), ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin((int)rows.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                    const Event& ev = *rows[rows.size() - 1 - i];     // newest first
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%.1f s", ev.seconds);
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(ev.added ? "+" : "-");
                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    if (ImGui::Selectable((ev.e.path + "." + ev.e.ext).c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) copy_row(ev.e.path.c_str());
                    ImGui::PopID();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(ev.e.cls.c_str());
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(ev.e.arc.c_str());
                }
            }
            ImGui::EndTable();
        }
    }

    ImGui::PopStyleVar();
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void FileTracker::on_config_load(const utility::Config& cfg) {
    mod_enabled   = cfg.get<bool>("file_tracker").value_or(false);
    scan_interval = cfg.get<int>("file_tracker_interval").value_or(scan_interval);
}

void FileTracker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("file_tracker", mod_enabled);
    cfg.set<int>("file_tracker_interval", scan_interval);
}
