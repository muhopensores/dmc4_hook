#include "SessionCheck.hpp"
#include "Lobby.hpp"
#include "../plugins/PluginHost.hpp"
#include "../../sdk/TestSwitch.hpp"

#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>

namespace session_check {
namespace {

std::string module_path(HMODULE module)
{
    wchar_t path[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(module, path, MAX_PATH);
    if (!n || n >= MAX_PATH) {
        return {};
    }
    char out[MAX_PATH * 3] = {};
    WideCharToMultiByte(CP_UTF8, 0, path, -1, out, sizeof out, nullptr, nullptr);
    return out;
}

// the file's content hash as 8 hex digits ("missing" when it can't be read)
std::string file_hash(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return "missing";
    }
    const std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    char hex[16];
    snprintf(hex, sizeof hex, "%08X", lobby::text_hash(bytes));
    return hex;
}

std::map<std::string, std::string> parse(const std::string& text)
{
    std::map<std::string, std::string> out;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        const size_t space = line.rfind(' ');
        if (space != std::string::npos && space > 0) {
            out[line.substr(0, space)] = line.substr(space + 1);
        }
    }
    return out;
}

std::string display(const std::string& name)
{
    if (name == "exe") {
        return "the game exe";
    }
    if (name == "dll") {
        return "dmc4_hook.dll";
    }
    return name;   // "plugin <file>"
}

} // namespace

const std::string& code_text()
{
    static const std::string text = [] {
        std::vector<std::string> lines;
        lines.push_back("exe " + file_hash(module_path(nullptr)));
        HMODULE self = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&code_text, &self);
        lines.push_back("dll " + file_hash(module_path(self)));
        for (const plugins::Info& p : plugins::list()) {
            if (p.state == "running") {   // switched off or failed: none of its code runs
                lines.push_back("plugin " + p.file + " " + file_hash("plugins\\" + p.file));
            }
        }
        if (test_switch("DMC4_TEST_CODE")) {   // test switch: a plugin only this machine has, for the check's refusal
            lines.push_back("plugin test_only_here.dll 00000000");
        }
        std::sort(lines.begin(), lines.end());
        std::string out;
        for (const std::string& l : lines) {
            out += l + "\n";
        }
        return out;
    }();
    return text;
}

std::vector<std::string> differences(const std::string& mine, const std::string& theirs, const char* them)
{
    const auto a = parse(mine), b = parse(theirs);
    std::vector<std::string> out;
    for (const auto& [name, hash] : a) {
        const auto it = b.find(name);
        if (it == b.end()) {
            out.push_back(display(name) + " (only here)");
        } else if (it->second != hash) {
            out.push_back(display(name));
        }
    }
    for (const auto& [name, hash] : b) {
        if (!a.count(name)) {
            out.push_back(display(name) + " (only on " + them + ")");
        }
    }
    return out;
}

} // namespace session_check
