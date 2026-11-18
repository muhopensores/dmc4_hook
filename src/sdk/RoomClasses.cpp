#include "RoomClasses.hpp"
#include "sArea.hpp"
#include "Devil4.hpp"
#include "EventScript.hpp"
#include <spdlog/spdlog.h>
#include <cstdio>
#include <cstring>
#include <deque>

// How a class is made (Vibed/RE/room_loading.md "Area registration"):
//   - a DTI node through the game's MtDTI constructor 0x8B69B0, parent = the base's node, so MtDTI::from finds the name
//     and createAreaInfo (which walks the tree right after create_all) records the class;
//   - the node's DTI vtable: the base's destructor and our newInstance (slot 1, thiscall on the node);
//   - the object vtable: the vanilla base's 15 slots with ours in 0 (destructor), 4 (getDTI), 5-8 (load / init / move /
//     final), 10 (getName), 11-14 (scripts, script player data, file names). A thunk finds the class from the object's vtable and walks the chain:
//     a class's callback, else its custom base's, else the vanilla slot.
// newInstance does what every vanilla room's does (alloc 0x1C0 from the unit allocator, aRoom::aRoom, vtable, +0x28 room,
// +0x2C stage), with the class's size; the vanilla destructor frees through the same allocator whatever the size.

namespace room_classes {
namespace {

constexpr uintptr_t kDtiCtor = 0x8B69B0;        // usercall eax = name, edi = parent, esi = node; push size, flags; retn 8
constexpr uintptr_t kDtiRoot = 0xE13FF4;        // MtDTI root
constexpr uintptr_t kRoomDti = 0xE55E68;        // aRoom::DTI
constexpr uintptr_t kRoomCtor = 0x409490;       // aRoom::aRoom, usercall esi = this, returns it
constexpr uintptr_t kUnitAllocator = 0xE14344;  // MtAllocator*: vfunc +0x14 alloc(size, align), +0x18 free
constexpr uintptr_t kAreaTable = 0x20;          // sArea AreaInfo entries, 28 bytes, +8 = the class's DTI
constexpr uintptr_t kAreaEntry = 28;
constexpr uintptr_t kAreaCount = 0x3820;
constexpr int kSlots = 15;

enum : int { SLOT_DTOR = 0, SLOT_GET_DTI = 4, SLOT_LOAD = 5, SLOT_INIT = 6, SLOT_MOVE = 7, SLOT_FINAL = 8,
             SLOT_GET_NAME = 10, SLOT_SCRIPT = 11, SLOT_AFTER_SCRIPT = 12, SLOT_SCRIPT_PLAYER = 13, SLOT_NAME_AT = 14 };

struct DtiNode {                                // MtDTI (sdk/MtDTI.hpp) as plain data
    void* vtable;
    const char* name;
    DtiNode* next;
    DtiNode* child;
    DtiNode* parent;
    DtiNode* link;
    uint32_t size_flags;
    uint32_t id;
};
static_assert(sizeof(DtiNode) == 0x20);

struct Class {
    ClassDesc desc;
    std::string name, label, error;
    bool created = false;
    int parent = -1;                // custom base class, -1 = a vanilla base
    void* vanilla[kSlots] = {};     // the vanilla base's slots (the nearest vanilla class in the chain)
    void* vtable[kSlots] = {};      // this class's
    void* dti_vtable[2] = {};
    DtiNode node = {};
    int room = -1, stage = -1;
    uint32_t fields_offset = kRoomSize, size = kRoomSize;
    int live = 0;
    std::vector<uint8_t> script_bytes, after_script_bytes;   // assembled from the desc's text (kept: the game reads them)
};

std::deque<Class> g_classes;        // a deque: nodes and names stay put
bool g_created = false;

using Fn0 = uintptr_t(__thiscall*)(void*);
using FnI = uintptr_t(__thiscall*)(void*, int);
using FnDtor = void*(__thiscall*)(void*, unsigned);
using FnNew = void*(__thiscall*)(void*);

DtiNode* find_dti(DtiNode* n, const char* name) {   // as MtDTI::getType 0x8B6AE0: depth first, names only
    for (DtiNode* c = n ? n->child : nullptr; c; c = c->next) {
        if (c->name && std::strcmp(c->name, name) == 0) return c;
        if (DtiNode* r = find_dti(c, name)) return r;
    }
    return nullptr;
}

bool is_room_class(const DtiNode* n) {
    for (; n; n = n->parent) if (reinterpret_cast<uintptr_t>(n) == kRoomDti) return true;
    return false;
}

int by_vtable(const void* vt) {
    for (size_t i = 0; i < g_classes.size(); ++i)
        if (g_classes[i].created && vt == g_classes[i].vtable) return (int)i;
    return -1;
}

int by_node(const void* node) {
    for (size_t i = 0; i < g_classes.size(); ++i)
        if (g_classes[i].created && node == &g_classes[i].node) return (int)i;
    return -1;
}

Ctx make_ctx(int level, aRoom* room, int slot, int arg) {
    const Class& k = g_classes[level];
    return Ctx{room, reinterpret_cast<char*>(room) + k.fields_offset, k.desc.user, level, slot, arg, 0};
}

uintptr_t base_of(int level, int slot, aRoom* room, int arg);

uintptr_t dispatch(int level, int slot, aRoom* room, int arg) {
    Class& k = g_classes[level];
    const Callbacks& cb = k.desc.cb;
    Ctx c = make_ctx(level, room, slot, arg);
    switch (slot) {
    case SLOT_LOAD:  if (cb.load) return cb.load(c) ? 1 : 0; break;
    case SLOT_INIT:  if (cb.init) { cb.init(c); return c.ret; } break;
    case SLOT_MOVE:  if (cb.move) { cb.move(c); return c.ret; } break;
    case SLOT_FINAL: if (cb.final) { cb.final(c); return c.ret; } break;
    case SLOT_SCRIPT:        if (k.desc.script) return reinterpret_cast<uintptr_t>(k.desc.script); break;
    case SLOT_AFTER_SCRIPT:  if (k.desc.after_script) return reinterpret_cast<uintptr_t>(k.desc.after_script); break;
    case SLOT_SCRIPT_PLAYER: if (k.desc.script_player) return reinterpret_cast<uintptr_t>(k.desc.script_player); break;
    case SLOT_NAME_AT:
        if (!k.desc.names.empty())
            return arg >= 0 && arg < (int)k.desc.names.size() ? reinterpret_cast<uintptr_t>(k.desc.names[arg].c_str()) : 0;
        break;
    default: break;
    }
    return base_of(level, slot, room, arg);
}

uintptr_t base_of(int level, int slot, aRoom* room, int arg) {
    const Class& k = g_classes[level];
    if (k.parent >= 0) return dispatch(k.parent, slot, room, arg);
    void* f = k.vanilla[slot];
    if (slot == SLOT_NAME_AT) return reinterpret_cast<FnI>(f)(room, arg);
    const uintptr_t r = reinterpret_cast<Fn0>(f)(room);
    // the vanilla load (0x40A590) returns a bool in al, the rest of eax is leftovers: a load callback's `base(c) != 0`
    // read them as "loaded" on the first frame, and the room was built before its files (room 901, 2026-11-18)
    return slot == SLOT_LOAD ? (r & 0xFF) : r;
}

int class_or_log(aRoom* self, const char* what) {
    const int k = by_vtable(self->vtable);
    if (k < 0) spdlog::error("RoomClasses: {} on a room with an unknown vtable {}", what, self->vtable);
    return k;
}

// A missing rom\room\st<room>.arc: the loader marks it failed (+0x4C bit 0x10) and the vanilla load waits for bit 1
// forever (aRoom load state 4). These rooms may live on loose files only, so a failed arc is dropped.
void drop_failed_arc(aRoom* r) {
    cResource* a = r->room_arc;
    if (!a) return;
    const uint16_t f = *reinterpret_cast<const uint16_t*>(reinterpret_cast<const char*>(a) + 0x4C);
    if ((f & 1) == 0 && (f & 0x10) != 0) {
        devil4_sdk::release_resource(a);
        r->room_arc = nullptr;
        spdlog::info("RoomClasses: room {}: no rom\\room\\st{:03d}.arc, using loose files", r->room_no, r->room_no);
    }
}

// Thunks: thiscall as fastcall (ecx = this, edx unused).
uintptr_t __fastcall t_load(aRoom* self, void*) {
    const int k = class_or_log(self, "load");
    if (k < 0) return 1;
    drop_failed_arc(self);
    return dispatch(k, SLOT_LOAD, self, 0);
}
uintptr_t __fastcall t_init(aRoom* self, void*) {
    const int k = class_or_log(self, "init");
    return k < 0 ? 0 : dispatch(k, SLOT_INIT, self, 0);
}
uintptr_t __fastcall t_move(aRoom* self, void*) {
    const int k = class_or_log(self, "move");
    return k < 0 ? 0 : dispatch(k, SLOT_MOVE, self, 0);
}
uintptr_t __fastcall t_final(aRoom* self, void*) {
    const int k = class_or_log(self, "final");
    return k < 0 ? 0 : dispatch(k, SLOT_FINAL, self, 0);
}
uintptr_t __fastcall t_script(aRoom* self, void*) {
    const int k = class_or_log(self, "script");
    return k < 0 ? 0 : dispatch(k, SLOT_SCRIPT, self, 0);
}
uintptr_t __fastcall t_after_script(aRoom* self, void*) {
    const int k = class_or_log(self, "after script");
    return k < 0 ? 0 : dispatch(k, SLOT_AFTER_SCRIPT, self, 0);
}
uintptr_t __fastcall t_script_player(aRoom* self, void*) {
    const int k = class_or_log(self, "script player");
    return k < 0 ? 0 : dispatch(k, SLOT_SCRIPT_PLAYER, self, 0);
}
uintptr_t __fastcall t_name_at(aRoom* self, void*, int index) {
    const int k = class_or_log(self, "name");
    return k < 0 ? 0 : dispatch(k, SLOT_NAME_AT, self, index);
}
uintptr_t __fastcall t_get_dti(aRoom* self, void*) {
    const int k = class_or_log(self, "getDTI");
    return k < 0 ? 0 : reinterpret_cast<uintptr_t>(&g_classes[k].node);
}
uintptr_t __fastcall t_get_name(aRoom* self, void*) {
    const int k = class_or_log(self, "getName");
    return k < 0 ? 0 : reinterpret_cast<uintptr_t>(g_classes[k].name.c_str());
}

void* __fastcall t_dtor(aRoom* self, void*, unsigned flags) {
    const int k = class_or_log(self, "destructor");
    if (k < 0) return self;   // can't happen: our vtable is only on our objects
    for (int l = k; l >= 0; l = g_classes[l].parent) {
        if (g_classes[l].desc.cb.destroy) {
            Ctx c = make_ctx(l, self, SLOT_DTOR, 0);
            g_classes[l].desc.cb.destroy(c);
        }
    }
    --g_classes[k].live;
    return reinterpret_cast<FnDtor>(g_classes[k].vanilla[SLOT_DTOR])(self, flags);   // frees when flags & 1
}

void* alloc_room(uint32_t size) {
    const uintptr_t a = *reinterpret_cast<const uintptr_t*>(kUnitAllocator);
    if (!a) return nullptr;
    using FnAlloc = void*(__thiscall*)(uintptr_t, uint32_t, uint32_t);
    return reinterpret_cast<FnAlloc>((*reinterpret_cast<void***>(a))[5])(a, size, 16);
}

aRoom* construct_room(void* mem) {
    aRoom* r = nullptr;
    const uintptr_t fn = kRoomCtor;
    __asm {
        push esi
        mov esi, mem
        call fn
        mov r, eax
        pop esi
    }
    return r;
}

void* __fastcall t_new(DtiNode* node, void*) {
    const int k = by_node(node);
    if (k < 0) return nullptr;
    Class& c = g_classes[k];
    void* mem = alloc_room(c.size);
    if (!mem) return nullptr;
    aRoom* r = construct_room(mem);
    std::memset(reinterpret_cast<char*>(r) + kRoomSize, 0, c.size - kRoomSize);
    r->vtable = c.vtable;
    r->room_no = c.room;
    r->stage_no = c.stage;
    ++c.live;
    int chain[32];
    int n = 0;
    for (int l = k; l >= 0 && n < 32; l = g_classes[l].parent) chain[n++] = l;
    while (n--) {                    // base classes first
        if (g_classes[chain[n]].desc.cb.create) {
            Ctx ctx = make_ctx(chain[n], r, -1, 0);
            g_classes[chain[n]].desc.cb.create(ctx);
        }
    }
    return r;
}

void construct_dti(DtiNode* node, const char* name, DtiNode* parent, uint32_t class_size) {
    const uintptr_t fn = kDtiCtor;
    __asm {
        push esi
        push edi
        push 0              // flags (+0x1B): not abstract
        push class_size
        mov eax, name
        mov edi, parent
        mov esi, node
        call fn             // retn 8
        pop edi
        pop esi
    }
}

// The class, or the reason it wasn't made (in error). No C++ unwinding needed: create_guarded catches SEH.
bool create_one(Class& k) {
    DtiNode* root = reinterpret_cast<DtiNode*>(kDtiRoot);
    if (find_dti(root, k.name.c_str())) { k.error = "the game already has a class " + k.name; return false; }
    DtiNode* b = find_dti(root, k.desc.base.c_str());
    if (!b) { k.error = "no base class " + k.desc.base; return false; }
    if (!is_room_class(b)) { k.error = k.desc.base + " isn't a room class"; return false; }
    int base_room = -1, base_stage = -1;
    const int p = by_node(b);
    if (p >= 0) {
        const Class& q = g_classes[p];
        std::memcpy(k.vanilla, q.vanilla, sizeof k.vanilla);
        k.parent = p;
        k.fields_offset = q.size;
        base_room = q.room;
        base_stage = q.stage;
    } else {
        // one instance of the vanilla base for its vtable and numbers (createAreaInfo makes one of each right after)
        void* t = reinterpret_cast<FnNew>(reinterpret_cast<void**>(b->vtable)[1])(b);
        if (!t) { k.error = k.desc.base + " didn't construct"; return false; }
        aRoom* tmp = static_cast<aRoom*>(t);
        std::memcpy(k.vanilla, tmp->vtable, sizeof k.vanilla);
        base_room = tmp->room_no;
        base_stage = tmp->stage_no;
        reinterpret_cast<FnDtor>(k.vanilla[SLOT_DTOR])(tmp, 1);
        k.fields_offset = kRoomSize;
    }
    k.room = k.desc.room;
    k.stage = k.desc.stage >= 0 ? k.desc.stage : base_stage;
    if (k.stage < 0) { k.error = "no stage number (the base " + k.desc.base + " has none)"; return false; }
    k.size = (k.fields_offset + k.desc.fields_size + 15) & ~15u;
    (void)base_room;

    std::memcpy(k.vtable, k.vanilla, sizeof k.vtable);
    k.vtable[SLOT_DTOR] = reinterpret_cast<void*>(&t_dtor);
    k.vtable[SLOT_GET_DTI] = reinterpret_cast<void*>(&t_get_dti);
    k.vtable[SLOT_LOAD] = reinterpret_cast<void*>(&t_load);
    k.vtable[SLOT_INIT] = reinterpret_cast<void*>(&t_init);
    k.vtable[SLOT_MOVE] = reinterpret_cast<void*>(&t_move);
    k.vtable[SLOT_FINAL] = reinterpret_cast<void*>(&t_final);
    k.vtable[SLOT_GET_NAME] = reinterpret_cast<void*>(&t_get_name);
    k.vtable[SLOT_SCRIPT] = reinterpret_cast<void*>(&t_script);
    k.vtable[SLOT_AFTER_SCRIPT] = reinterpret_cast<void*>(&t_after_script);
    k.vtable[SLOT_SCRIPT_PLAYER] = reinterpret_cast<void*>(&t_script_player);
    k.vtable[SLOT_NAME_AT] = reinterpret_cast<void*>(&t_name_at);

    k.dti_vtable[0] = reinterpret_cast<void**>(b->vtable)[0];
    k.dti_vtable[1] = reinterpret_cast<void*>(&t_new);
    construct_dti(&k.node, k.name.c_str(), b, k.size);
    k.node.vtable = k.dti_vtable;
    k.created = true;
    return true;
}

int create_guarded(Class* k) {
    __try {
        return create_one(*k) ? 1 : 0;
    } __except (1) {
        return -1;
    }
}

uintptr_t area_instance() { return reinterpret_cast<uintptr_t>(devil4_sdk::get_sArea()); }

} // namespace

uintptr_t base(Ctx& c) {
    c.ret = base_of(c.level, c.slot, c.room, c.arg);
    return c.ret;
}

bool add(const ClassDesc& d, std::string* error) {
    auto fail = [&](std::string why) {
        spdlog::error("RoomClasses: {} room {}: {}", d.owner, d.room, why);
        if (error) *error = std::move(why);
        return false;
    };
    if (g_created) return fail("too late: room classes are made when the game starts (add them at startup)");
    if (d.room < 0 || d.room > 0xFFEF) return fail("room numbers go from 0 to 65519");
    if (d.room == 600) return fail("room 600 is special-cased by the game (no room is built)");
    char name[32];
    std::snprintf(name, sizeof name, "aRoom%03d", d.room);
    for (const Class& k : g_classes)
        if (k.name == name) return fail(std::string(name) + " is already added by " + k.desc.owner);
    std::vector<uint8_t> script, after_script;
    std::string why;
    if (!d.script_text.empty() && !event_script::assemble(d.script_text, script, &why)) return fail("load script: " + why);
    if (!d.after_script_text.empty() && !event_script::assemble(d.after_script_text, after_script, &why))
        return fail("per-frame script: " + why);
    Class& k = g_classes.emplace_back();
    k.desc = d;
    k.name = name;
    k.label = std::string(name) + " (" + d.owner + ")";
    k.script_bytes = std::move(script);
    k.after_script_bytes = std::move(after_script);
    if (!k.script_bytes.empty()) k.desc.script = k.script_bytes.data();
    if (!k.after_script_bytes.empty()) k.desc.after_script = k.after_script_bytes.data();
    return true;
}

void create_all() {
    if (g_created) return;
    g_created = true;
    for (Class& k : g_classes) {
        const int r = create_guarded(&k);
        if (r < 0) k.error = "crashed while making the class";
        if (r == 1) spdlog::info("RoomClasses: {} : {} (room {}, stage {}, {} bytes) for {}", k.name, k.desc.base, k.room,
                                 k.stage, k.size, k.desc.owner);
        else spdlog::error("RoomClasses: {} not made: {}", k.name, k.error);
    }
}

bool created() { return g_created; }

void clear_callbacks(const std::string& owner) {
    for (Class& k : g_classes) {
        if (k.desc.owner != owner) continue;
        k.desc.cb = Callbacks{};
        k.desc.script = k.desc.after_script = k.desc.script_player = nullptr;   // the owner's memory
        if (!k.script_bytes.empty()) k.desc.script = k.script_bytes.data();    // ours (assembled text) stay
        if (!k.after_script_bytes.empty()) k.desc.after_script = k.after_script_bytes.data();
        k.desc.user = nullptr;
    }
}

bool is_custom(int room) {
    for (const Class& k : g_classes) if (k.created && k.room == room) return true;
    return false;
}

const char* label(int room) {
    for (const Class& k : g_classes) if (k.created && k.room == room) return k.label.c_str();
    return nullptr;
}

aRoom* current() {
    sArea* a = devil4_sdk::get_sArea();
    return a ? a->aRoomPtr : nullptr;
}

int class_of(const aRoom* room) { return room ? by_vtable(room->vtable) : -1; }

void* fields_of(aRoom* room) {
    const int k = class_of(room);
    return k < 0 ? nullptr : reinterpret_cast<char*>(room) + g_classes[k].fields_offset;
}

std::vector<Info> list() {
    std::vector<Info> out;
    const uintptr_t area = area_instance();
    const int count = area ? *reinterpret_cast<const int*>(area + kAreaCount) : 0;
    for (const Class& k : g_classes) {
        Info i;
        i.name = k.name;
        i.base = k.desc.base;
        i.owner = k.desc.owner;
        i.error = k.error;
        i.room = k.created ? k.room : k.desc.room;
        i.stage = k.stage;
        i.size = k.size;
        i.created = k.created;
        i.live = k.live;
        for (int j = 0; k.created && j < count && j < 512; ++j)
            if (*reinterpret_cast<void* const*>(area + kAreaTable + kAreaEntry * j + 8) == &k.node) i.in_area_table = true;
        out.push_back(std::move(i));
    }
    return out;
}

} // namespace room_classes
