#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

// The debug server's access to game memory (Memory.cpp): reads and writes that fail instead of crashing (VirtualQuery
// + SEH), the address form the tools share (address / path / offset), value types, the DTI tree, and raw calls.
// Game thread for anything that touches game objects.
namespace debug_server::mem {

using json = nlohmann::json;

bool safe_read(uintptr_t a, void* dst, size_t n);
bool safe_write(uintptr_t a, const void* src, size_t n);   // makes read-only / code pages writable for the copy
template <typename T> bool rd(uintptr_t a, T& v) { return safe_read(a, &v, sizeof(T)); }
uintptr_t rd_ptr(uintptr_t a);                              // 0 on a bad read
json safe_cstr(uintptr_t a, size_t max = 256);              // null on a bad pointer
bool executable(uintptr_t a);

std::string hex(uintptr_t v);
std::string lower(std::string s);
bool parse_number(const json& j, long long& out);           // an integer or a string ("0x8AF03F", "-4")

// Named starting points: sArea, sUnit, sMediator, sRender, room, player1-4 (throws on an unknown name).
uintptr_t root_address(const std::string& name);
// args.address (number, hex string or root), args.path (pointers to follow), args.offset (added last), as `read`;
// the addresses passed through go into *chain. Throws with the reason.
uintptr_t resolve(const json& args, json* chain = nullptr);

// value types
enum class Kind { U8, I8, U16, I16, U32, I32, U64, I64, F32, F64, Ptr, CStr, Vec3, Vec4, Mat4 };
struct TypeInfo {
    const char* name;
    Kind kind;
    size_t size;   // in memory (vec3 = 16, as MtVector3)
};
const TypeInfo* type_named(const std::string& n);
std::string scalar_type_names();                            // "u8, i8, ..."
json value_at(uintptr_t a, const TypeInfo& t);             // throws on a bad read
void encode(const json& v, const TypeInfo& t, uint8_t* out);   // a value as `value_at` gives it -> bytes (throws)

// DTI (MtDTI as plain data)
constexpr uintptr_t kDtiRoot = 0xE13FF4;
struct Dti {
    uintptr_t vtable, name, next, child, parent, link;
    uint32_t size_flags, id;
};
static_assert(sizeof(Dti) == 0x20);
inline bool read_dti(uintptr_t a, Dti& d) { return a && rd(a, d); }
std::string dti_name(uintptr_t dti);
uintptr_t dti_of(uintptr_t obj);                            // the object's class (vtable slot 4, getDTI), 0 if none
bool dti_is_a(uintptr_t dti, const std::string& base_lower);
uintptr_t find_dti(const std::string& name);                // any case, 0 if none
// depth first, as MtDTI::getType; f(node, depth) returns false to stop; visits at most `budget` nodes
template <typename F> void walk_dti(uintptr_t node, F&& f, int& budget, int depth = 0) {
    Dti d;
    if (!read_dti(node, d)) return;
    for (uintptr_t c = d.child; c && budget > 0;) {
        Dti cd;
        if (!read_dti(c, cd)) return;
        --budget;
        if (!f(c, depth + 1)) return;
        walk_dti(c, f, budget, depth + 1);
        c = cd.next;
    }
}

// raw calls (the `call` tool)
enum class Conv { Cdecl, Stdcall, Thiscall, Fastcall };
struct CallResult {
    bool ok = false;           // false: the call raised an exception (code)
    uint32_t code = 0;
    uintptr_t at = 0;          // where
    uint32_t eax = 0, edx = 0;
    double st0 = 0;            // only with float_return
};
// args[0] goes to ecx for thiscall / fastcall (args[1] to edx for fastcall), the rest on the stack, last pushed first
CallResult call(uintptr_t fn, Conv conv, const uint32_t* args, int count, bool float_return);

} // namespace debug_server::mem
