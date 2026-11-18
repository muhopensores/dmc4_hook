#include "Memory.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/Players.hpp"
#include "../../sdk/RoomClasses.hpp"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace debug_server::mem {

// ---- safe memory ----

bool readable(uintptr_t a, size_t n) {
    if (a < 0x10000 || n == 0) return n == 0 && a >= 0x10000;
    const uintptr_t end = a + n;
    if (end < a) return false;
    while (a < end) {
        MEMORY_BASIC_INFORMATION m;
        if (!VirtualQuery(reinterpret_cast<void*>(a), &m, sizeof(m)) || m.State != MEM_COMMIT) return false;
        const DWORD p = m.Protect;
        if (p & (PAGE_GUARD | PAGE_NOACCESS)) return false;
        if (!(p & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                   PAGE_EXECUTE_WRITECOPY)))
            return false;
        a = reinterpret_cast<uintptr_t>(m.BaseAddress) + m.RegionSize;
    }
    return true;
}

bool copy_seh(void* dst, uintptr_t src, size_t n) {
    __try {
        std::memcpy(dst, reinterpret_cast<const void*>(src), n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool safe_read(uintptr_t a, void* dst, size_t n) { return readable(a, n) && copy_seh(dst, a, n); }

uintptr_t rd_ptr(uintptr_t a) {
    uintptr_t v = 0;
    return rd(a, v) ? v : 0;
}

// a C string at a (null on a bad pointer), at most max characters
json safe_cstr(uintptr_t a, size_t max) {
    if (!a) return json();
    std::string s;
    char buf[64];
    while (s.size() < max) {
        const size_t n = std::min<size_t>(sizeof(buf), 0x1000 - ((a + s.size()) & 0xFFF));   // stay within a page
        if (!safe_read(a + s.size(), buf, n)) return s.empty() ? json() : json(s);
        const void* z = std::memchr(buf, 0, n);
        if (z) {
            s.append(buf, static_cast<const char*>(z) - buf);
            return s;
        }
        s.append(buf, n);
    }
    s.resize(max);
    return s;
}

std::string hex(uintptr_t v) {
    char b[16];
    std::snprintf(b, sizeof(b), "0x%08X", static_cast<unsigned>(v));
    return b;
}

std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// a number given as an integer or a string ("0x8AF03F", "1234", "-4")
bool parse_number(const json& j, long long& out) {
    if (j.is_number_integer()) { out = j.get<long long>(); return true; }
    if (j.is_number_unsigned()) { out = (long long)j.get<unsigned long long>(); return true; }
    if (!j.is_string()) return false;
    const std::string s = j.get<std::string>();
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtoll(s.c_str(), &end, 0);
    return end && *end == 0;
}


// ---- DTI ----

std::string dti_name(uintptr_t a) {
    Dti d;
    if (!read_dti(a, d)) return {};
    const json n = safe_cstr(d.name, 128);
    return n.is_string() ? n.get<std::string>() : std::string();
}

// the object's class node: vtable slot 4 (getDTI), called under SEH
uintptr_t call_get_dti(uintptr_t fn, uintptr_t obj) {
    __try {
        using Fn = uintptr_t(__thiscall*)(void*);
        return reinterpret_cast<Fn>(fn)(reinterpret_cast<void*>(obj));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

uintptr_t dti_of(uintptr_t obj) {
    const uintptr_t vt = rd_ptr(obj);
    if (!vt) return 0;
    const uintptr_t fn = rd_ptr(vt + 0x10);
    if (!fn || !executable(fn)) return 0;
    return call_get_dti(fn, obj);
}

bool dti_is_a(uintptr_t node, const std::string& base_lower) {
    for (int i = 0; node && i < 64; ++i) {
        Dti d;
        if (!read_dti(node, d)) return false;
        if (lower(dti_name(node)) == base_lower) return true;
        node = d.parent;
    }
    return false;
}

uintptr_t find_dti(const std::string& name) {
    if (lower(name) == lower(dti_name(kDtiRoot))) return kDtiRoot;
    const std::string want = lower(name);
    uintptr_t found = 0;
    int budget = 50000;
    walk_dti(kDtiRoot, [&](uintptr_t n, int) {
        if (lower(dti_name(n)) == want) { found = n; return false; }
        return true;
    }, budget);
    return found;
}


// ---- named roots ----

uintptr_t root_address(const std::string& name) {
    const std::string n = lower(name);
    if (n == "sarea") return reinterpret_cast<uintptr_t>(devil4_sdk::get_sArea());
    if (n == "sunit") return reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
    if (n == "smediator") return reinterpret_cast<uintptr_t>(devil4_sdk::get_sMediator());
    if (n == "srender") return reinterpret_cast<uintptr_t>(devil4_sdk::get_sRender());
    if (n == "room") return reinterpret_cast<uintptr_t>(room_classes::current());
    if (n.size() == 7 && n.compare(0, 6, "player") == 0 && n[6] >= '1' && n[6] <= '4')
        return player_info::player(n[6] - '1');
    throw std::runtime_error("no root '" + name + "' (roots: sArea, sUnit, sMediator, sRender, room, player1-4)");
}

// ---- typed reads ----

const TypeInfo kTypes[] = {
    {"u8", Kind::U8, 1},     {"i8", Kind::I8, 1},     {"u16", Kind::U16, 2}, {"i16", Kind::I16, 2},
    {"u32", Kind::U32, 4},   {"i32", Kind::I32, 4},   {"u64", Kind::U64, 8}, {"i64", Kind::I64, 8},
    {"f32", Kind::F32, 4},   {"f64", Kind::F64, 8},   {"ptr", Kind::Ptr, 4}, {"cstr", Kind::CStr, 4},
    {"vec3", Kind::Vec3, 16}, {"vec4", Kind::Vec4, 16}, {"mat4", Kind::Mat4, 64},
};

const TypeInfo* type_named(const std::string& n) {
    for (const TypeInfo& t : kTypes)
        if (n == t.name) return &t;
    return nullptr;
}

json value_at(uintptr_t a, const TypeInfo& t) {
    uint8_t b[64];
    if (!safe_read(a, b, t.size)) throw std::runtime_error("can't read " + std::to_string(t.size) + " bytes at " + hex(a));
    auto f = [&](int i) { float v; std::memcpy(&v, b + 4 * i, 4); return v; };
    switch (t.kind) {
    case Kind::U8: return b[0];
    case Kind::I8: return (int8_t)b[0];
    case Kind::U16: { uint16_t v; std::memcpy(&v, b, 2); return v; }
    case Kind::I16: { int16_t v; std::memcpy(&v, b, 2); return v; }
    case Kind::U32: { uint32_t v; std::memcpy(&v, b, 4); return v; }
    case Kind::I32: { int32_t v; std::memcpy(&v, b, 4); return v; }
    case Kind::U64: { uint64_t v; std::memcpy(&v, b, 8); return v; }
    case Kind::I64: { int64_t v; std::memcpy(&v, b, 8); return v; }
    case Kind::F32: return f(0);
    case Kind::F64: { double v; std::memcpy(&v, b, 8); return v; }
    case Kind::Ptr: { uint32_t v; std::memcpy(&v, b, 4); return hex(v); }
    case Kind::CStr: { uint32_t v; std::memcpy(&v, b, 4); return safe_cstr(v); }
    case Kind::Vec3: return json::array({f(0), f(1), f(2)});
    case Kind::Vec4: return json::array({f(0), f(1), f(2), f(3)});
    case Kind::Mat4: {
        json rows = json::array();
        for (int r = 0; r < 4; ++r) rows.push_back(json::array({f(4 * r), f(4 * r + 1), f(4 * r + 2), f(4 * r + 3)}));
        return rows;
    }
    }
    return json();
}


// ---- writes ----

bool writable_now(uintptr_t a, size_t n) {
    const uintptr_t end = a + n;
    while (a < end) {
        MEMORY_BASIC_INFORMATION m;
        if (!VirtualQuery(reinterpret_cast<void*>(a), &m, sizeof(m)) || m.State != MEM_COMMIT) return false;
        if (!(m.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) || (m.Protect & PAGE_GUARD)) return false;
        a = reinterpret_cast<uintptr_t>(m.BaseAddress) + m.RegionSize;
    }
    return true;
}

bool write_seh(uintptr_t dst, const void* src, size_t n) {
    __try {
        std::memcpy(reinterpret_cast<void*>(dst), src, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool safe_write(uintptr_t a, const void* src, size_t n) {
    if (!readable(a, n)) return false;
    if (writable_now(a, n)) return write_seh(a, src, n);
    DWORD old = 0;   // read-only data or code: open the pages for the copy (as utility::Patch does)
    if (!VirtualProtect(reinterpret_cast<void*>(a), n, PAGE_EXECUTE_READWRITE, &old)) return false;
    const bool ok = write_seh(a, src, n);
    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(a), n, old, &ignored);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(a), n);
    return ok;
}

bool executable(uintptr_t a) {
    MEMORY_BASIC_INFORMATION m;
    return a && VirtualQuery(reinterpret_cast<void*>(a), &m, sizeof(m)) && m.State == MEM_COMMIT &&
           (m.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) &&
           !(m.Protect & PAGE_GUARD);
}

std::string scalar_type_names() {
    std::string s;
    for (const TypeInfo& t : kTypes) s += std::string(s.empty() ? "" : ", ") + t.name;
    return s;
}

namespace {

double number(const json& v) {
    if (v.is_number()) return v.get<double>();
    long long n = 0;
    if (parse_number(v, n)) return (double)n;
    throw std::runtime_error("expected a number, got " + v.dump());
}

long long integer(const json& v) {
    long long n = 0;
    if (parse_number(v, n)) return n;
    if (v.is_number_float()) {
        const double d = v.get<double>();
        if (d == std::floor(d)) return (long long)d;
    }
    throw std::runtime_error("expected an integer, got " + v.dump());
}

void floats(const json& v, size_t count, uint8_t* out) {
    if (!v.is_array() || v.size() != count) throw std::runtime_error("expected " + std::to_string(count) + " numbers");
    for (size_t i = 0; i < count; ++i) {
        const float f = (float)number(v[i]);
        std::memcpy(out + 4 * i, &f, 4);
    }
}

} // namespace

void encode(const json& v, const TypeInfo& t, uint8_t* out) {
    switch (t.kind) {
    case Kind::U8: case Kind::I8: { const auto x = (uint8_t)integer(v); std::memcpy(out, &x, 1); return; }
    case Kind::U16: case Kind::I16: { const auto x = (uint16_t)integer(v); std::memcpy(out, &x, 2); return; }
    case Kind::U32: case Kind::I32: case Kind::Ptr: { const auto x = (uint32_t)integer(v); std::memcpy(out, &x, 4); return; }
    case Kind::U64: case Kind::I64: { const auto x = (uint64_t)integer(v); std::memcpy(out, &x, 8); return; }
    case Kind::F32: { const float x = (float)number(v); std::memcpy(out, &x, 4); return; }
    case Kind::F64: { const double x = number(v); std::memcpy(out, &x, 8); return; }
    case Kind::CStr: throw std::runtime_error("cstr can't be written (write the string's bytes, or a ptr)");
    case Kind::Vec3: floats(v, 3, out); return;   // the 4th float keeps what `out` held (the caller reads it first)
    case Kind::Vec4: floats(v, 4, out); return;
    case Kind::Mat4:
        if (!v.is_array() || v.size() != 4) throw std::runtime_error("mat4: 4 rows of 4 numbers");
        for (int r = 0; r < 4; ++r) floats(v[r], 4, out + 16 * r);
        return;
    }
}

uintptr_t resolve(const json& a, json* chain) {
    if (!a.contains("address")) throw std::runtime_error("address is required");
    uintptr_t addr = 0;
    long long n = 0;
    if (parse_number(a["address"], n)) addr = (uintptr_t)n;
    else if (a["address"].is_string()) addr = root_address(a["address"].get<std::string>());
    else throw std::runtime_error("address: a number, a hex string or a root name");
    if (!addr) throw std::runtime_error("the root is null now");
    if (chain) chain->push_back(hex(addr));
    if (a.contains("path")) {
        for (const json& o : a["path"]) {
            if (!parse_number(o, n)) throw std::runtime_error("path: numbers or hex strings");
            const uintptr_t at = addr + (uintptr_t)n;
            uintptr_t p = 0;
            if (!rd(at, p)) throw std::runtime_error("can't read the pointer at " + hex(at));
            if (!p) throw std::runtime_error("null pointer at " + hex(at));
            addr = p;
            if (chain) chain->push_back(hex(addr));
        }
    }
    if (a.contains("offset")) {
        if (!parse_number(a["offset"], n)) throw std::runtime_error("offset: a number or hex string");
        addr += (uintptr_t)n;
    }
    return addr;
}

// ---- raw calls ----

namespace {

// No C++ objects here (SEH and inline asm). esp comes back from saved_esp, so cdecl and stdcall clean up alike.
void raw_call(uintptr_t fn, const uint32_t* stack_args, int stack_count, uint32_t ecx_value, uint32_t edx_value,
              bool float_return, uint32_t* r_eax, uint32_t* r_edx, double* r_st0) {
    uint32_t saved_esp = 0, out_eax = 0, out_edx = 0;
    double out_st0 = 0;
    __asm {
        mov saved_esp, esp
        mov esi, stack_args
        mov edi, stack_count
    push_next:
        test edi, edi
        jz pushed
        dec edi
        push dword ptr [esi + edi * 4]
        jmp push_next
    pushed:
        mov ecx, ecx_value
        mov edx, edx_value
        call fn
        mov out_eax, eax
        mov out_edx, edx
        mov esp, saved_esp
    }
    if (float_return) {
        __asm fstp out_st0
    }
    *r_eax = out_eax;
    *r_edx = out_edx;
    *r_st0 = out_st0;
}

int call_filter(EXCEPTION_POINTERS* e, uint32_t* code, uintptr_t* at) {
    *code = e->ExceptionRecord->ExceptionCode;
    *at = reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_EXECUTE_HANDLER;
}

bool call_seh(uintptr_t fn, const uint32_t* stack_args, int stack_count, uint32_t ecx_value, uint32_t edx_value,
              bool float_return, uint32_t* r_eax, uint32_t* r_edx, double* r_st0, uint32_t* code, uintptr_t* at) {
    __try {
        raw_call(fn, stack_args, stack_count, ecx_value, edx_value, float_return, r_eax, r_edx, r_st0);
        return true;
    } __except (call_filter(GetExceptionInformation(), code, at)) {
        return false;
    }
}

} // namespace

CallResult call(uintptr_t fn, Conv conv, const uint32_t* args, int count, bool float_return) {
    CallResult r;
    uint32_t ecx_value = 0, edx_value = 0;
    int first = 0;
    if ((conv == Conv::Thiscall || conv == Conv::Fastcall) && count > 0) ecx_value = args[first++];
    if (conv == Conv::Fastcall && count > 1) edx_value = args[first++];
    r.ok = call_seh(fn, args + first, count - first, ecx_value, edx_value, float_return, &r.eax, &r.edx, &r.st0, &r.code,
                    &r.at);
    return r;
}

} // namespace debug_server::mem
