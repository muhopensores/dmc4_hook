#include "PluginRooms.hpp"
#include "PluginHost.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../../sdk/RoomClasses.hpp"
#include "../../sdk/EventScript.hpp"
#include "../../sdk/sArea.hpp"
#include "../../mods/AreaJump.hpp"
#include <algorithm>
#include <cstring>
#include <deque>

// Each registered class is a RoomClasses class whose callbacks are the trampolines below, with `user` = its PluginRoom.
// A trampoline runs the plugin's callback through run_guarded: a stopped plugin isn't called, and a crash stops it.
// Either way the room's own step (`base`) still runs if the callback didn't get to call it, so the room goes on as its
// base class. The descriptors are copied (the plugin's strings and arrays needn't outlive init).

namespace plugins {
namespace {

struct PluginRoom {
    dmc4_plugin* self = nullptr;
    dmc4_room_class desc = {};   // the plugin's callbacks and user (strings copied into the ClassDesc)
};
std::deque<PluginRoom> g_rooms;   // a deque: RoomClasses keeps pointers to them

// One callback call: what base() needs, and whether the plugin already ran the base step.
struct Call {
    PluginRoom* r = nullptr;
    room_classes::Ctx* ctx = nullptr;
    dmc4_room_call pub = {};
    bool based = false;
    int result = 0;
};

void __cdecl call_create(void* u) { auto* c = static_cast<Call*>(u); c->r->desc.create(&c->pub); }
void __cdecl call_destroy(void* u) { auto* c = static_cast<Call*>(u); c->r->desc.destroy(&c->pub); }
void __cdecl call_load(void* u) { auto* c = static_cast<Call*>(u); c->result = c->r->desc.load(&c->pub); }
void __cdecl call_init(void* u) { auto* c = static_cast<Call*>(u); c->r->desc.init(&c->pub); }
void __cdecl call_move(void* u) { auto* c = static_cast<Call*>(u); c->r->desc.move(&c->pub); }
void __cdecl call_final(void* u) { auto* c = static_cast<Call*>(u); c->r->desc.final(&c->pub); }

Call make_call(room_classes::Ctx& ctx) {
    Call c;
    c.r = static_cast<PluginRoom*>(ctx.user);
    c.ctx = &ctx;
    c.pub.room = ctx.room;
    c.pub.fields = ctx.fields;
    c.pub.user = c.r ? c.r->desc.user : nullptr;
    return c;
}

// run the plugin's callback, then the base step if the callback didn't (not there, plugin stopped, or it crashed first)
bool run_step(room_classes::Ctx& ctx, bool has, dmc4_callback fn, Call& c, const char* what) {
    c.pub.internal = &c;
    const bool ran = has && run_guarded(c.r->self, fn, &c, what);
    if (!ran && !c.based) room_classes::base(ctx);
    return ran;
}

void t_create(room_classes::Ctx& ctx) {
    Call c = make_call(ctx);
    c.pub.internal = &c;
    if (c.r && c.r->desc.create) run_guarded(c.r->self, call_create, &c, "a room's create callback");
}
void t_destroy(room_classes::Ctx& ctx) {
    Call c = make_call(ctx);
    c.pub.internal = &c;
    if (c.r && c.r->desc.destroy) run_guarded(c.r->self, call_destroy, &c, "a room's destroy callback");
}
bool t_load(room_classes::Ctx& ctx) {
    Call c = make_call(ctx);
    if (run_step(ctx, c.r->desc.load != nullptr, call_load, c, "a room's load callback"))
        return c.result != 0;
    return ctx.ret != 0;   // the base step's result
}
void t_init(room_classes::Ctx& ctx) {
    Call c = make_call(ctx);
    run_step(ctx, c.r->desc.init != nullptr, call_init, c, "a room's init callback");
}
void t_move(room_classes::Ctx& ctx) {
    Call c = make_call(ctx);
    run_step(ctx, c.r->desc.move != nullptr, call_move, c, "a room's move callback");
}
void t_final(room_classes::Ctx& ctx) {
    Call c = make_call(ctx);
    run_step(ctx, c.r->desc.final != nullptr, call_final, c, "a room's final callback");
}

int __cdecl api_register_class(dmc4_plugin* self, const dmc4_room_class* in) {
    if (!self || !in) return 0;
    dmc4_room_class d = {};                                    // an older plugin's smaller struct: the rest stays zero
    std::memcpy(&d, in, std::min<size_t>(in->size ? in->size : sizeof d, sizeof d));
    d.size = sizeof d;
    room_classes::ClassDesc cd;
    cd.owner = "plugin " + plugin_file(self);
    cd.room = d.room;
    cd.base = d.base ? d.base : "aRoom";
    cd.stage = d.stage;
    cd.fields_size = d.fields_size;
    if (d.script) cd.script_text = d.script;
    if (d.after_script) cd.after_script_text = d.after_script;
    for (int i = 0; d.names && i < d.name_count; ++i) cd.names.emplace_back(d.names[i] ? d.names[i] : "");
    PluginRoom& r = g_rooms.emplace_back();
    r.self = self;
    r.desc = d;
    r.desc.base = nullptr;   // the plugin's strings aren't kept
    r.desc.script = r.desc.after_script = nullptr;
    r.desc.names = nullptr;
    cd.user = &r;
    cd.cb.create = t_create;
    cd.cb.destroy = t_destroy;
    cd.cb.load = t_load;
    cd.cb.init = t_init;
    cd.cb.move = t_move;
    cd.cb.final = t_final;
    std::string why;
    if (!room_classes::add(cd, &why)) {
        g_rooms.pop_back();
        plugin_log(self, DMC4_LOG_ERROR, "room " + std::to_string(d.room) + " not registered: " + why);
        return 0;
    }
    plugin_log(self, DMC4_LOG_INFO, "room " + std::to_string(d.room) + " registered (base " + cd.base + ")");
    return 1;
}

int __cdecl api_base(const dmc4_room_call* call) {
    if (!call || !call->internal) return 0;
    auto* c = static_cast<Call*>(call->internal);
    if (c->pub.internal != c || !c->ctx) return 0;   // not a call the hook made
    c->based = true;
    return room_classes::base(*c->ctx) != 0 ? 1 : 0;
}

void* __cdecl api_current() { return room_classes::current(); }
int __cdecl api_room_number(void* room) { return room ? static_cast<aRoom*>(room)->room_no : -1; }
int __cdecl api_stage_number(void* room) { return room ? static_cast<aRoom*>(room)->stage_no : -1; }
void* __cdecl api_fields(void* room) { return room_classes::fields_of(static_cast<aRoom*>(room)); }
int __cdecl api_enter(int room) { return AreaJump::jump_to_room_id(room) ? 1 : 0; }

int __cdecl api_check_script(const char* text, char* error, uint32_t error_size) {
    std::vector<uint8_t> bytes;
    std::string why;
    const bool ok = text && event_script::assemble(text, bytes, &why);
    if (!text) why = "no text";
    if (error && error_size) {
        const size_t n = ok ? 0 : std::min<size_t>(why.size(), error_size - 1);
        std::memcpy(error, why.data(), n);
        error[n] = 0;
    }
    return ok ? 1 : 0;
}

const dmc4_rooms_api g_rooms_api = {
    sizeof(dmc4_rooms_api),
    api_register_class,
    api_base,
    api_current,
    api_room_number,
    api_stage_number,
    api_fields,
    api_enter,
    api_check_script,
};

} // namespace

const dmc4_rooms_api* rooms_api() { return &g_rooms_api; }

} // namespace plugins
