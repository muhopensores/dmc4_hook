#include "ModelShape.hpp"
#include "Devil4.hpp"
#include <unordered_map>

namespace model_shape {

namespace {
constexpr uintptr_t kDtiShape       = 0x00EAEF00;   // rShape::DTI (.shp)
constexpr uintptr_t kDtiShapeWeight = 0x00EAEEE0;   // rShapeWeight::DTI (.shw)
constexpr uintptr_t fn_set_shape    = 0x00A68260;   // cShape mpShape setter (this@ecx, rsrc), retn 4: addRefs, builds the maps
constexpr uintptr_t fn_set_weight   = 0x00A67AF0;   // cShape::WeightNode mpWeight setter, same convention: addRefs,
                                                    // mFrameMax = the .shw's frame count, on when curves >= targets
constexpr uintptr_t kShape = 0xC04;                 // uModel::mShape
constexpr uintptr_t kDelta = 0x10;                  // cUnit: the frame delta uDevil4Model::move passes to the update
// cShape
constexpr uintptr_t kKeyWeight = 0x0C, kRes = 0x10, kWeightList = 0x58, kNodeSize = 0x20;
// cShape::WeightNode
constexpr uintptr_t kNodeWeight = 0x08, kNodeSpeed = 0x0C, kNodeRate = 0x10, kNodeFrame = 0x14, kNodeAttr = 0x1C,
                    kNodeState = 0x1E;
constexpr uintptr_t kShpTargets = 0x70;             // rShape: target count

template <class T> T& at(uintptr_t p, uintptr_t o) { return *(T*)(p + o); }

std::unordered_map<uintptr_t, uint8_t> g_retire;    // per model: the lists to stop at their end (bit = list)

void retire_flag(uintptr_t model, int list, bool on) {
    if (on) { g_retire[model] |= (uint8_t)(1 << list); return; }
    auto it = g_retire.find(model);
    if (it == g_retire.end()) return;
    it->second &= (uint8_t)~(1 << list);
    if (!it->second) g_retire.erase(it);
}

void set_resource(uintptr_t setter, uintptr_t obj, uintptr_t rsrc) {
    ((void(__fastcall*)(uintptr_t, int, uintptr_t))setter)(obj, 0, rsrc);    // __thiscall, callee pops its argument
}

uintptr_t load(uintptr_t dti, const std::string& path) {
    if (path.empty()) return 0;
    return (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)dti, (char*)path.c_str(), 1);
}

uintptr_t node(uintptr_t model, int list) { return model + kShape + kWeightList + kNodeSize * list; }

void settings(uintptr_t n, const List& l) {
    at<float>(n, kNodeSpeed) = l.speed;
    at<float>(n, kNodeRate)  = l.rate;
    at<float>(n, kNodeFrame) = l.frame;
    at<uint16_t>(n, kNodeAttr) = (at<uint16_t>(n, kNodeAttr) & ~1) | (l.loop ? 0 : 1);   // bit 0: once
}
} // namespace

bool set(uintptr_t model, const char* shp, const List* lists, int count, std::string& error) {
    if (!model || !shp || !*shp) { error = "no model or .shp"; return false; }
    if (count > kLists) count = kLists;
    const uintptr_t res = load(kDtiShape, shp);
    if (!res) { error = std::string(shp) + ".shp didn't load"; return false; }
    uintptr_t weights[kLists] = {};
    for (int i = 0; i < count; ++i) {
        if (lists[i].shw.empty()) continue;
        weights[i] = load(kDtiShapeWeight, lists[i].shw);
        if (!weights[i]) {
            error = lists[i].shw + ".shw didn't load";
            devil4_sdk::release_resource((cResource*)res);
            for (int k = 0; k < i; ++k) if (weights[k]) devil4_sdk::release_resource((cResource*)weights[k]);
            return false;
        }
    }
    const uintptr_t shape = model + kShape;
    g_retire.erase(model);
    set_resource(fn_set_shape, shape, res);          // first: a list's setter checks its curves against the targets
    devil4_sdk::release_resource((cResource*)res);
    for (int i = 0; i < kLists; ++i) {
        const uintptr_t n = node(model, i);
        set_resource(fn_set_weight, n, weights[i]);
        if (!weights[i]) continue;
        devil4_sdk::release_resource((cResource*)weights[i]);
        settings(n, lists[i]);
    }
    return true;
}

bool play(uintptr_t model, int list, const List& l, std::string& error) {
    if (list < 0 || list >= kLists) { error = "list " + std::to_string(list) + " (0-3)"; return false; }
    if (!shape_of(model)) { error = "the model has no shape (.shp) set"; return false; }
    const uintptr_t w = load(kDtiShapeWeight, l.shw);
    if (!w) { error = l.shw + ".shw didn't load"; return false; }
    const uintptr_t n = node(model, list);
    set_resource(fn_set_weight, n, w);               // a different file: set and restarted; the same one: nothing happens
    devil4_sdk::release_resource((cResource*)w);     // the list keeps its own reference
    at<uint16_t>(n, kNodeState) &= ~2;               // "reached the end" (a list played once that finished)
    settings(n, l);
    retire_flag(model, list, !l.loop && !l.hold);
    if (!list_on(model, list)) { error = l.shw + " stayed off (fewer curves than the .shp has targets)"; return false; }
    return true;
}

void stop(uintptr_t model, int list) {
    if (!model || list < 0 || list >= kLists) return;
    retire_flag(model, list, false);
    set_resource(fn_set_weight, node(model, list), 0);
}

void retire(uintptr_t model) {
    auto it = g_retire.find(model);
    if (it == g_retire.end()) return;
    const uint8_t mask = it->second;
    for (int i = 0; i < kLists; ++i)
        if ((mask & (1 << i)) && (at<uint16_t>(node(model, i), kNodeState) & 2)) stop(model, i);
}

void forget(uintptr_t model) { g_retire.erase(model); }

void clear(uintptr_t model) {
    if (!model) return;
    g_retire.erase(model);
    for (int i = 0; i < kLists; ++i) set_resource(fn_set_weight, node(model, i), 0);
    set_resource(fn_set_shape, model + kShape, 0);
}

// sub_A68770(this@edi, float delta), retn 4
void update(uintptr_t model, float delta) {
    if (!shape_of(model)) return;
    uintptr_t shape = model + kShape;
    uintptr_t update_call = 0xA68770;
    _asm {
        pushad
        mov edi, shape
        push delta
        call update_call
        popad
    }
}

uintptr_t shape_of(uintptr_t model) { return model ? at<uintptr_t>(model + kShape, kRes) : 0; }

int targets(uintptr_t model) {
    const uintptr_t res = shape_of(model);
    return res ? (int)at<uint32_t>(res, kShpTargets) : 0;
}

float frame(uintptr_t model, int list) { return at<float>(node(model, list), kNodeFrame); }
float speed(uintptr_t model, int list) { return at<float>(node(model, list), kNodeSpeed); }

bool list_on(uintptr_t model, int list) {
    const uintptr_t n = node(model, list);
    return at<uintptr_t>(n, kNodeWeight) && !(at<uint16_t>(n, kNodeState) & 1);
}

float weight(uintptr_t model, int target) {
    const uintptr_t w = at<uintptr_t>(model + kShape, kKeyWeight);
    return (w && target >= 0 && target < targets(model)) ? ((const float*)w)[target] : 0.0f;
}

float delta_of(uintptr_t model) { return at<float>(model, kDelta); }

} // namespace model_shape
