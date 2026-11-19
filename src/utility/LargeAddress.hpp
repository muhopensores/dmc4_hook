#pragma once
// Large address awareness of the game's exe. DevilMayCry4_DX9.exe isn't large address aware, so the game gets 2 GB of
// address space; with the flag (IMAGE_FILE_LARGE_ADDRESS_AWARE in the PE file header) 64-bit Windows gives it 4 GB.
// Windows reads the flag when the process starts, so it has to be set in the exe on disk and takes effect on the next
// start (setting it in memory does nothing). Steam's DRM wrapper (SteamStub, a `.bind` section) checks the exe, so a
// wrapped exe is left alone: Steamless removes the wrapper and leaves a plain exe that can take the flag.
#include <Windows.h>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace large_address {

// This run has the flag.
inline bool running() {
    const auto* base = reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr));
    const auto* dos  = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt   = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    return (nt->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
}

inline std::wstring exe_path() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size());
        if (n == 0) {
            return {};
        }
        if (n < path.size()) {
            path.resize(n);
            return path;
        }
        path.resize(path.size() * 2);
    }
}

inline std::string to_utf8(const std::wstring& s) {
    if (s.empty()) {
        return {};
    }
    const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n, nullptr, nullptr);
    return out;
}

struct ExeState {
    bool readable   = false; // the exe on disk is a 32-bit x86 PE
    bool flagged    = false; // it has the flag (if this run doesn't, the next start will)
    bool steam_stub = false; // it has a `.bind` section: Steam's DRM wrapper
};

namespace detail {

inline bool read_file(const std::wstring& path, std::vector<char>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !out.empty();
}

// The file header's offset in the file, or 0 when the bytes aren't a 32-bit x86 PE.
inline size_t file_header_offset(const std::vector<char>& b) {
    IMAGE_DOS_HEADER dos;
    if (b.size() < sizeof(dos)) {
        return 0;
    }
    std::memcpy(&dos, b.data(), sizeof(dos));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || (size_t)dos.e_lfanew + sizeof(IMAGE_NT_HEADERS32) > b.size()) {
        return 0;
    }
    IMAGE_NT_HEADERS32 nt;
    std::memcpy(&nt, b.data() + dos.e_lfanew, sizeof(nt));
    if (nt.Signature != IMAGE_NT_SIGNATURE || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
        return 0;
    }
    return (size_t)dos.e_lfanew + offsetof(IMAGE_NT_HEADERS32, FileHeader);
}

inline IMAGE_FILE_HEADER file_header(const std::vector<char>& b, size_t at) {
    IMAGE_FILE_HEADER fh;
    std::memcpy(&fh, b.data() + at, sizeof(fh));
    return fh;
}

inline bool has_section(const std::vector<char>& b, size_t at, const char* name) {
    const IMAGE_FILE_HEADER fh = file_header(b, at);
    size_t sec = at + sizeof(IMAGE_FILE_HEADER) + fh.SizeOfOptionalHeader;
    for (WORD i = 0; i < fh.NumberOfSections; ++i, sec += sizeof(IMAGE_SECTION_HEADER)) {
        if (sec + sizeof(IMAGE_SECTION_HEADER) > b.size()) {
            return false;
        }
        IMAGE_SECTION_HEADER sh;
        std::memcpy(&sh, b.data() + sec, sizeof(sh));
        if (std::strncmp(reinterpret_cast<const char*>(sh.Name), name, IMAGE_SIZEOF_SHORT_NAME) == 0) {
            return true;
        }
    }
    return false;
}

inline std::string last_error_text(DWORD code) {
    char* text = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
                   0, reinterpret_cast<char*>(&text), 0, nullptr);
    std::string out = text ? text : "error " + std::to_string(code);
    LocalFree(text);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == '.')) {
        out.pop_back();
    }
    return out;
}

inline bool exists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

} // namespace detail

inline ExeState read_exe() {
    ExeState state;
    std::vector<char> bytes;
    if (!detail::read_file(exe_path(), bytes)) {
        return state;
    }
    const size_t at = detail::file_header_offset(bytes);
    if (!at) {
        return state;
    }
    state.readable   = true;
    state.flagged    = (detail::file_header(bytes, at).Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
    state.steam_stub = detail::has_section(bytes, at, ".bind");
    return state;
}

// Sets the flag in the exe on disk. Windows lets a running exe be renamed but not written, so the flagged bytes go to a
// new file, the current exe is renamed to `<name>_no_laa.exe` (`_no_laa2.exe`, ... when that exists) and the new file
// takes its name. On success `backup` is the old exe's file name; on failure `error` says why and nothing changed.
inline bool write_flagged(std::wstring& backup, std::string& error) {
    const std::wstring path = exe_path();
    std::vector<char> bytes;
    if (path.empty() || !detail::read_file(path, bytes)) {
        error = "couldn't read the game's exe";
        return false;
    }
    const size_t at = detail::file_header_offset(bytes);
    if (!at) {
        error = "the game's exe isn't a 32-bit Windows program";
        return false;
    }
    if (detail::has_section(bytes, at, ".bind")) {
        error = "the game's exe still has Steam's DRM wrapper: run Steamless on it first";
        return false;
    }
    IMAGE_FILE_HEADER fh = detail::file_header(bytes, at);
    if (fh.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) {
        error = "the game's exe already has the flag: restart the game";
        return false;
    }
    fh.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    std::memcpy(bytes.data() + at, &fh, sizeof(fh));

    const size_t dot        = path.find_last_of(L'.');
    const std::wstring stem = dot == std::wstring::npos ? path : path.substr(0, dot);
    const std::wstring ext  = dot == std::wstring::npos ? L"" : path.substr(dot);
    std::wstring old_path   = stem + L"_no_laa" + ext;
    for (int i = 2; detail::exists(old_path); ++i) {
        old_path = stem + L"_no_laa" + std::to_wstring(i) + ext;
    }
    const std::wstring new_path = stem + L"_laa.tmp";

    {
        std::ofstream f(new_path, std::ios::binary | std::ios::trunc);
        f.write(bytes.data(), (std::streamsize)bytes.size());
        f.close();
        if (!f) {
            const DWORD code = GetLastError();
            DeleteFileW(new_path.c_str());
            error = "couldn't write in the game folder (" + detail::last_error_text(code) +
                    "): start the game as administrator once, or move the game out of Program Files";
            return false;
        }
    }
    if (!MoveFileExW(path.c_str(), old_path.c_str(), 0)) {
        const DWORD code = GetLastError();
        DeleteFileW(new_path.c_str());
        error = "couldn't rename the game's exe (" + detail::last_error_text(code) + ")";
        return false;
    }
    if (!MoveFileExW(new_path.c_str(), path.c_str(), 0)) {
        const DWORD code = GetLastError();
        MoveFileExW(old_path.c_str(), path.c_str(), 0);
        DeleteFileW(new_path.c_str());
        error = "couldn't put the new exe in place (" + detail::last_error_text(code) + ")";
        return false;
    }
    const size_t slash = old_path.find_last_of(L"\\/");
    backup             = slash == std::wstring::npos ? old_path : old_path.substr(slash + 1);
    return true;
}

} // namespace large_address
