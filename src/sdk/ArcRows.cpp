#include "ArcRows.hpp"
#include "Devil4.hpp"
#include "MtMemoryStream.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>

namespace arc_rows {

namespace {

constexpr uintptr_t kArchiveNewInstance = 0x008E1500;   // rArchive::MyDTI::newInstance: no arguments, eax = the arc (0x70 bytes) or 0
constexpr uintptr_t kPath = 0x04, kAttr = 0x48, kState = 0x4C, kSize = 0x50, kId = 0x58;   // cResource
constexpr uintptr_t kArcRows = 0x60, kArcCount = 0x64;   // rArchive: cResource* rows[], count
constexpr uint32_t  kAttrSyncRows = 0x200;               // rArchive::load creates every row before returning
constexpr uint32_t  kArcData = 0x8000;                   // where the data starts in the game's arcs
constexpr uint16_t  kArcVersion = 17;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

#pragma pack(push, 1)
struct TocRow {
    char     path[64];
    uint32_t dti, stored, size, offset;   // size: bits 0-28 decompressed size, 29-31 quality tier (kept as the source has it)
};
#pragma pack(pop)
static_assert(sizeof(TocRow) == 0x50, "arc TOC row");

// Read the rows out of their source arcs and lay them out as one ARC: header, TOC (the source rows with new offsets),
// data from kArcData, each row's stored bytes unchanged. Rows keep their order (a nested list after its members: the
// game's ARCS loader only looks its ids up).
bool build_arc(const std::vector<Row>& want_rows, std::vector<uint8_t>& out, std::string& error) {
    const int count = (int)want_rows.size();
    std::vector<TocRow> toc(count);
    std::vector<std::vector<uint8_t>> data(count);
    std::map<std::string, std::vector<int>> by_arc;
    for (int i = 0; i < count; ++i) by_arc[want_rows[i].arc].push_back(i);

    for (const auto& [arc, wanted] : by_arc) {
        const std::string file = std::string("nativePC\\") + arc;
        FILE* f = nullptr;
        if (fopen_s(&f, file.c_str(), "rb") != 0 || !f) { error = "can't open " + file; return false; }
        char magic[4] = {};
        uint16_t version = 0, count = 0;
        const bool head_ok = fread(magic, 1, 4, f) == 4 && fread(&version, 2, 1, f) == 1 && fread(&count, 2, 1, f) == 1;
        if (!head_ok || memcmp(magic, "ARC\0", 4) != 0 || version != kArcVersion) {
            fclose(f);
            error = file + " is not a DX9 arc (ARC v17)";
            return false;
        }
        std::vector<TocRow> rows(count);
        if (fread(rows.data(), sizeof(TocRow), count, f) != count) { fclose(f); error = "short TOC in " + file; return false; }
        for (int i : wanted) {
            const Row& want = want_rows[i];
            const TocRow* hit = nullptr;
            for (const TocRow& r : rows)
                if (r.dti == want.dti && _strnicmp(r.path, want.path, sizeof(r.path)) == 0) { hit = &r; break; }
            if (!hit) { fclose(f); error = std::string(want.path) + " is not in " + file + " (another version of the arc?)"; return false; }
            data[i].resize(hit->stored);
            if (fseek(f, (long)hit->offset, SEEK_SET) != 0 || fread(data[i].data(), 1, hit->stored, f) != hit->stored) {
                fclose(f);
                error = std::string("can't read ") + want.path + " from " + file;
                return false;
            }
            toc[i] = *hit;
        }
        fclose(f);
    }

    size_t total = kArcData;
    for (const auto& d : data) total += d.size();
    out.assign(total, 0);
    memcpy(out.data(), "ARC\0", 4);
    *reinterpret_cast<uint16_t*>(out.data() + 4) = kArcVersion;
    *reinterpret_cast<uint16_t*>(out.data() + 6) = (uint16_t)count;
    uint32_t off = kArcData;
    for (int i = 0; i < count; ++i) {
        toc[i].offset = off;
        memcpy(out.data() + 8 + i * sizeof(TocRow), &toc[i], sizeof(TocRow));
        memcpy(out.data() + off, data[i].data(), data[i].size());
        off += (uint32_t)data[i].size();
    }
    return true;
}

} // namespace

bool load(const std::string& name, const std::vector<Row>& rows_in, Result& st) {
    if (st.arc) return true;
    st = Result{};
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<uint8_t> bytes;
    if (!build_arc(rows_in, bytes, st.error)) {
        spdlog::warn("ArcRows: {}: {}", name, st.error);
        return false;
    }
    const uintptr_t arc = reinterpret_cast<uintptr_t(*)()>(kArchiveNewInstance)();   // refcount 1, ours
    if (!arc) { st.error = "rArchive allocation failed"; return false; }
    std::snprintf(&at<char>(arc, kPath), 64, "%s", name.c_str());       // a name for FileTracker / debuggers
    at<uint64_t>(arc, kId) = 0;                                            // not in sResource's table
    at<uint32_t>(arc, kAttr) |= kAttrSyncRows;
    MtMemoryStream stream(bytes.data(), (uint32_t)bytes.size());
    const bool ok = load_resource_from((void*)arc, stream);                // rArchive::load 0x8E17B0
    const uintptr_t rows = at<uintptr_t>(arc, kArcRows);
    const uint32_t  n    = at<uint32_t>(arc, kArcCount);
    for (uint32_t k = 0; rows && k < n; ++k) {
        const uintptr_t r = at<uintptr_t>(rows, 4 * k);
        if (!r) { ++st.failed; continue; }
        ++st.rows;
    }
    st.failed += (int)rows_in.size() - (int)n;
    st.bytes = (uint32_t)bytes.size();
    st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (!ok || st.failed) {
        st.error = !ok ? "the game refused the arc (a row's class not found?)" : std::to_string(st.failed) + " rows not created";
        spdlog::warn("ArcRows: {}: {} ({} rows, {} failed)", name, st.error, st.rows, st.failed);
        devil4_sdk::release_resource((cResource*)arc);                     // its unload releases the rows it made
        st.arc = 0;
        return false;
    }
    at<uint32_t>(arc, kSize) = st.bytes;                                   // as create_file_load: size, state "loaded"
    at<uint32_t>(arc, kState) |= 1;
    st.arc = arc;
    spdlog::info("ArcRows: {} loaded: {} rows, {} bytes, {:.1f} ms", name, st.rows, st.bytes, st.ms);
    return true;
}

void release(Result& st) {
    if (st.arc) devil4_sdk::release_resource((cResource*)st.arc);         // last reference: rows released (vtbl +0x2C)
    st = Result{};
}

} // namespace arc_rows
