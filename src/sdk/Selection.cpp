#include "Selection.hpp"
#include <atomic>
#include <mutex>

namespace selection {
namespace {

std::mutex g_mutex;
uintptr_t g_unit = 0;
By g_by = By::None;
float g_point[3] = {};
bool g_have_point = false;
std::atomic<uint32_t> g_revision{ 1 };

} // namespace

const char* by_name(By by) {
    switch (by) {
    case By::Id: return "id";
    case By::Joint: return "joint";
    case By::List: return "list";
    case By::Tool: return "tool";
    default: return "none";
    }
}

void select(uintptr_t unit, By by) {
    std::lock_guard lock(g_mutex);
    g_unit = unit;
    g_by = unit ? by : By::None;
    ++g_revision;
}

void clear() { select(0, By::None); }

uintptr_t unit() {
    std::lock_guard lock(g_mutex);
    return g_unit;
}

By by() {
    std::lock_guard lock(g_mutex);
    return g_by;
}

uint32_t revision() { return g_revision.load(); }

void set_point(const float p[3]) {
    std::lock_guard lock(g_mutex);
    g_point[0] = p[0];
    g_point[1] = p[1];
    g_point[2] = p[2];
    g_have_point = true;
    ++g_revision;
}

bool point(float out[3]) {
    std::lock_guard lock(g_mutex);
    if (!g_have_point) return false;
    out[0] = g_point[0];
    out[1] = g_point[1];
    out[2] = g_point[2];
    return true;
}

void forget_point() {
    std::lock_guard lock(g_mutex);
    g_have_point = false;
    ++g_revision;
}

} // namespace selection
