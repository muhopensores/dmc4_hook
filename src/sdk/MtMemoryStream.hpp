#pragma once
#include <cstdint>

// MtMemoryStream (DX9 vftable 0xBFF448; MtDTI size 0x14, parent MtStream): the game's stream over a memory buffer.
// Resource loaders read through the stream's vtable (+0x30 read(dst, n), +0x44 size, +0x48 seek(offset, origin
// 0 set / 1 current / 2 end); +0x20 returns false: not asynchronous, so rArchive::load takes its synchronous path), so a
// resource can be loaded from bytes in memory instead of a file: build one around a buffer that outlives the call and
// pass its address where the game wants an MtStream*. RE: Vibed\RE\sResource_findings.md.
struct MtMemoryStream {
    uintptr_t   vtable = 0x00BFF448;
    const void* data   = nullptr;
    uint32_t    pos    = 0;
    uint32_t    size   = 0;
    uint32_t    flags  = 1;      // 1 readable (4 would make the stream free `data` through TempAllocator: never on our buffers)

    MtMemoryStream(const void* bytes, uint32_t count) : data(bytes), size(count) {}
};
static_assert(sizeof(MtMemoryStream) == 0x14, "MtMemoryStream layout");

// cResource::load (vtbl slot 8, +0x20; __thiscall): fills `rsrc` from `stream`. True on success.
inline bool load_resource_from(void* rsrc, MtMemoryStream& stream) {
    void** vt = *reinterpret_cast<void***>(rsrc);
    return reinterpret_cast<bool(__thiscall*)(void*, void*)>(vt[8])(rsrc, &stream);
}
