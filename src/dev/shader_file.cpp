#include "shader_file.hpp"
#include "../mod.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace shader_file {

namespace {

enum class State { BuiltIn, Same, Differs };

struct Entry {
    Source source;
    // what was on disk when it was last compared (the file is read again only when these change)
    bool exists = false;
    std::filesystem::file_time_type time{};
    std::uintmax_t size = 0;
    State state = State::BuiltIn;
    std::string message;        // the last action's result
};

std::vector<Entry>& entries() {
    static std::vector<Entry> list;
    return list;
}

State refresh(Entry& e) {
    std::error_code ec;
    const std::filesystem::path p(e.source.path);
    const bool exists = std::filesystem::is_regular_file(p, ec);
    if (!exists) {
        e.exists = false;
        return e.state = State::BuiltIn;
    }
    const auto time = std::filesystem::last_write_time(p, ec);
    const auto size = std::filesystem::file_size(p, ec);
    if (e.exists && time == e.time && size == e.size) return e.state;
    e.exists = true;
    e.time = time;
    e.size = size;
    std::ifstream f(p, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    // an empty file is skipped by the compilers' read_source, which then use the built-in source
    if (text.empty()) return e.state = State::BuiltIn;
    return e.state = (text == e.source.embedded) ? State::Same : State::Differs;
}

// <path> -> <path>.bak (an older .bak goes)
bool put_aside(const Entry& e) {
    std::error_code ec;
    const std::filesystem::path p(e.source.path);
    std::filesystem::path bak = p;
    bak += ".bak";
    std::filesystem::remove(bak, ec);
    std::filesystem::rename(p, bak, ec);
    return !ec;
}

bool write(Entry& e) {
    if (e.exists && e.state == State::Differs && !put_aside(e)) return false;
    std::error_code ec;
    const std::filesystem::path p(e.source.path);
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(e.source.embedded, (std::streamsize)std::strlen(e.source.embedded));
    f.close();
    if (!f) return false;
    e.source.recompile();
    return true;
}

} // namespace

bool add(const Source& source) {
    auto& list = entries();
    Entry e;
    e.source = source;
    list.insert(std::upper_bound(list.begin(), list.end(), e,
                                 [](const Entry& a, const Entry& b) { return a.source.order < b.source.order; }),
                e);
    return true;
}

void draw() {
    ImGui::PushID("ShaderFiles");
    ImGui::TextDisabled(_("Each shader runs its built-in source unless its file is there."));
    ImGui::SameLine();
    Mod::help_marker(_("PBR materials, the soft shadows and the tone map compile their shaders when the game starts. While "
                       "a shader's file (the path in its tooltip) exists, it is compiled instead of the built-in source: "
                       "edit it and press Reload to try a change without a build. A file from an older dmc4_hook keeps the "
                       "newer built-in shader out (\"differs\" in orange): Write file replaces it, Use built in puts it "
                       "aside. Either way the old file is kept next to it as .hlsl.bak. Compiled shaders are cached in a "
                       "cache folder next to the file; it can be deleted at any time."));
    const ImVec4 orange(1.0f, 0.6f, 0.3f, 1.0f);
    for (Entry& e : entries()) {
        ImGui::PushID(e.source.path);
        const State st = refresh(e);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(e.source.name);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", e.source.path);
        ImGui::SameLine(160.0f);
        switch (st) {
        case State::BuiltIn: ImGui::TextDisabled(_("built in")); break;
        case State::Same:    ImGui::TextUnformatted(_("file, same as built in")); break;
        case State::Differs: ImGui::TextColored(orange, _("file, differs from built in")); break;
        }
        if (st == State::Differs && ImGui::IsItemHovered())
            ImGui::SetTooltip(_("Edited, or written by an older dmc4_hook. It runs instead of the built-in shader, so "
                                "anything the built-in one gained since is missing."));
        ImGui::SameLine(380.0f);
        bool first = true;
        auto button = [&](const char* label) {
            if (!first) ImGui::SameLine();
            first = false;
            return ImGui::SmallButton(label);
        };
        if (st != State::Same && button(_("Write file")))
            e.message = write(e) ? "" : _("couldn't write the file");
        if (st != State::BuiltIn && button(_("Reload"))) e.source.recompile();
        if (st != State::BuiltIn && button(_("Use built in"))) {
            if (put_aside(e)) {
                e.message.clear();
                e.source.recompile();
            } else {
                e.message = _("couldn't move the file");
            }
        }
        if (!e.message.empty()) ImGui::TextColored(orange, "%s", e.message.c_str());
        ImGui::PopID();
    }
    ImGui::PopID();
}

} // namespace shader_file
