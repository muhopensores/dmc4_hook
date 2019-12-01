#ifndef HACKLIB_PATTERNSCANNER_H
#define HACKLIB_PATTERNSCANNER_H

#include "hacklib/Memory.h"
#include "hacklib/ExeFile.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>


namespace hl
{
/// A helper for doing batch searches through memory regions.
class PatternScanner
{
public:
    PatternScanner();
    /// Searches for referenced strings in code of the module.
    std::vector<uintptr_t> find(const std::vector<std::string>& strings, const std::string& moduleName = "");
    /// Searches for one specific referenced string within the code of a module.
    uintptr_t findString(const std::string& str, const std::string& moduleName = "", int instance = 0);
    /// Searches for referenced string in code of the module.
    /// \param strings The query strings.
    /// \param moduleName The name of the target module or empty string for main module.
    /// \return A map from query strings to result addresses.
    std::map<std::string, uintptr_t> findMap(const std::vector<std::string>& strings,
                                             const std::string& moduleName = "");

private:
    std::unordered_map<std::string, hl::ModuleHandle> moduleMap;
    std::unordered_map<std::string, std::unique_ptr<hl::ExeFile>> exeFileMap;
    std::unordered_map<std::string, bool> verifyRelocsMap;
    std::vector<hl::MemoryRegion> memoryMap;
};


/// Finds a binary pattern with mask in executable sections of a module.
/// The mask is a string containing 'x' to match and '?' to ignore.
/// If moduleName is nullptr the module of the main module is searched.
uintptr_t FindPatternMask(const char* byteMask, const char* checkMask, const std::string& moduleName = "",
                          int instance = 0);
/// Variant for arbitrary memory.
uintptr_t FindPatternMask(const char* byteMask, const char* checkMask, uintptr_t address, size_t len, int instance = 0);
/// More convenient and less error prone alternative.
/// Example: "12 45 ?? 89 ?? ?? ?? cd ef"
uintptr_t FindPattern(const std::string& pattern, const std::string& moduleName = "", int instance = 0);
/// \overload
uintptr_t FindPattern(const std::string& pattern, uintptr_t address, size_t len, int instance = 0);

/// Helper to follow relative addresses in instructions. For example in jumps and calls.
/// \param adr The memory address of the relative address within an instruction
/// \param trail The number of trailing bytes after the relative address until the next instruction.
/// \return The resolved absolute address.
uintptr_t FollowRelativeAddress(uintptr_t adr, int trail = 0);

/// Returns a vector of hl::MemoryRegion%s.
const std::vector<hl::MemoryRegion>& GetCodeRegions(const std::string& moduleName = "");
}

#endif
