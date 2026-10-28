#include "XmlMove.hpp"
#include "../../sdk/Enums_Internal.hpp"   // CANCEL_NO
#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace xmlmove {

namespace {

bool ieq(const char* a, const char* b) { return a && b && _stricmp(a, b) == 0; }

// decimal or 0x hex; `def` when missing / unparsable
long num(const char* text, long def) {
    if (!text || !*text) return def;
    char* end = nullptr;
    const long v = std::strtol(text, &end, 0);
    return end == text ? def : v;
}
long num(const pugi::xml_attribute& a, long def) { return a ? num(a.as_string(), def) : def; }
float fnum(const pugi::xml_attribute& a, float def) { return a ? a.as_float(def) : def; }
bool bnum(const pugi::xml_attribute& a, bool def) { return a ? a.as_bool(def) : def; }

// "bank:slot" (either part decimal or 0x hex) -> bank << 8 | slot; -1 if missing
int parse_motion(const char* text) {
    if (!text || !*text) return -1;
    unsigned bank = 0, slot = 0;
    if (sscanf_s(text, "%i:%i", &bank, &slot) != 2) return -1;
    return (int)((bank << 8) | (slot & 0xFF));
}

struct Name { const char* name; long value; };

long lookup(const Name* tbl, size_t n, const char* text, long def) {
    if (!text || !*text) return def;
    for (size_t i = 0; i < n; ++i) if (ieq(tbl[i].name, text)) return tbl[i].value;
    return num(text, def);
}

const Name kButtons[]   = { {"melee", 0x1}, {"gun", 0x2}, {"jump", 0x4}, {"lockon", 0x10}, {"lock-on", 0x10}, {"style", 0x200} };
const Name kBtnModes[]  = { {"trg", 1}, {"press", 1}, {"rel", 2}, {"release", 2}, {"on", 3}, {"held", 3} };
const Name kLevers[]    = { {"centred", 0}, {"centered", 0}, {"forward", 1}, {"back", 4}, {"any", 15} };
const Name kLeverModes[] = { {"target45", 1}, {"target70", 2}, {"screen", 3} };
const Name kConditions[] = { {"none", 0}, {"lockon", 1}, {"lock-on", 1}, {"pose", 1}, {"dash", 2}, {"laser", 3} };
const Name kAttrs[]     = { {"melee", 0}, {"short", 0}, {"gun", 1}, {"long", 1}, {"command", 2}, {"proper", 4} };
const Name kAir[]       = { {"ground", 1}, {"air", 2}, {"both", 3}, {"fly", 0x40} };
const Name kKinds[]     = { {"open", 0x10}, {"close", 0x20}, {"buffer", 0x30}, {"reserve", 0x30}, {"open_alt", 0x30} };
const Name kMotAttrs[]  = { {"none", 0}, {"null_trans_off", 1} };
const Name kCancels[]   = { {"C_NORM0", C_NORM0}, {"C_NORM1", C_NORM1}, {"C_NORM2", C_NORM2}, {"C_COMMAND", C_COMMAND},
                            {"C_JUMP", C_JUMP}, {"C_CHOUHATSU", C_CHOUHATSU}, {"C_GUN", C_GUN}, {"C_ALL", C_ALL} };
const char* kDirs[DIR_COUNT] = { "forward", "down", "back", "left", "right", "up" };
const Name kContactBytes[] = { {"atck", 0}, {"free2", 1}, {"camvib", 2}, {"cam", 2}, {"free1", 3}, {"free0", 4}, {"padvib", 5}, {"pad", 5} };

#define N(tbl) tbl, sizeof(tbl) / sizeof(tbl[0])

} // namespace

namespace {
bool parse_command(const char* path, const pugi::xml_node& c, Command& cm) {
    cm.present    = true;
    cm.name       = c.attribute("name").as_string();
    cm.condition  = (uint32_t)lookup(N(kConditions), c.attribute("condition").as_string(), 0);
    cm.predicate  = c.attribute("predicate").as_string();
    cm.side_angle = fnum(c.attribute("side_angle"), 50.0f);
    cm.active     = (int16_t)num(c.attribute("active"), 1);
    if (cm.name.empty()) { spdlog::warn("MoveDef: {}: <command> needs a name", path); return false; }
    for (pugi::xml_node s : c.children()) {
        CommandStep st;
        if (ieq(s.name(), "btn") || ieq(s.name(), "button")) {
            st.type  = 1;
            st.btn   = button_bits(s.attribute("id").as_string());
            st.mode  = (uint16_t)lookup(N(kBtnModes), s.attribute("mode").as_string(), 1);
            st.timer = (int16_t)num(s.attribute("timer"), 0);
        } else if (ieq(s.name(), "lever")) {
            st.type  = 2;
            st.lever = (uint16_t)lookup(N(kLevers), s.attribute("dir").as_string(), 0);
            st.mode  = (uint16_t)lookup(N(kLeverModes), s.attribute("mode").as_string(), 1);
            st.timer = (int16_t)num(s.attribute("timer"), -1);
        } else {
            spdlog::warn("MoveDef: {}: unknown <command> step <{}> ignored", path, s.name());
            continue;
        }
        cm.steps.push_back(st);
    }
    return true;
}

const Name kKnocks[] = { {"direction", 0}, {"facing", 1}, {"away", 2} };

// <shell atk= entry= kind= vsattr= unitline= radius= length= height= life= knock= knock_angle= one_hit= show= + overrides>
void parse_shell(const pugi::xml_node& n, int character, Shell& s) {
    const char* atk = n.attribute("atk").as_string(character == 1 ? "Collision\\pl006" : "Collision\\pl000");
    strncpy_s(s.atk, atk, sizeof s.atk - 1);
    s.entry       = (int)num(n.attribute("entry"), 0);
    s.kind        = (int)num(n.attribute("kind"), 1);
    s.vsattr      = (uint32_t)num(n.attribute("vsattr"), 0);
    s.unit_line   = (int)num(n.attribute("unitline"), 0x12);
    s.radius      = fnum(n.attribute("radius"), 60.0f);
    s.length      = fnum(n.attribute("length"), 120.0f);
    s.height      = fnum(n.attribute("height"), 80.0f);
    s.life        = fnum(n.attribute("life"), 90.0f);
    s.knock       = (int)lookup(N(kKnocks), n.attribute("knock").as_string(), 0);
    s.knock_angle = fnum(n.attribute("knock_angle"), 0.0f);
    s.one_hit     = bnum(n.attribute("one_hit"), false);
    s.show        = bnum(n.attribute("show"), false);
    s.damage_type   = (int)num(n.attribute("damage_type"), -1);
    s.damage_type_b = (int)num(n.attribute("damage_type_b"), -1);
    s.level_i       = (int)num(n.attribute("level_i"), -1);
    s.level_b       = (int)num(n.attribute("level_b"), -1);
    s.attack_id     = (int)num(n.attribute("attack_id"), -1);
    s.range_type    = (int)num(n.attribute("range_type"), -1);
    s.hit_stop      = (int)num(n.attribute("hit_stop"), -1);
    s.hitmark       = (int)num(n.attribute("hitmark"), -1);
    s.hit_se        = (int)num(n.attribute("hit_se"), -1);
    s.attack_flag   = (int)num(n.attribute("attack_flag"), -1);
    s.blown_angle   = (int)num(n.attribute("blown_angle"), -1);
    s.element       = (int)num(n.attribute("element"), -1);
    s.damage        = fnum(n.attribute("damage"), -1.0f);
    s.stylish_point = fnum(n.attribute("stylish_point"), -1.0f);
    s.stylish_timer = fnum(n.attribute("stylish_timer"), -1.0f);
    s.dt_add        = fnum(n.attribute("dt_add"), -1.0f);
}
} // namespace

int dir_from_name(const char* name) {
    for (int i = 0; i < DIR_COUNT; ++i) if (ieq(kDirs[i], name)) return i;
    return -1;
}
const char* dir_name(int dir) { return dir >= 0 && dir < DIR_COUNT ? kDirs[dir] : "?"; }
uint32_t button_bits(const char* name) { return (uint32_t)lookup(N(kButtons), name, 0); }
int cancel_category(const char* name) { return (int)lookup(N(kCancels), name, -1); }
int contact_byte(const char* name) { return (int)lookup(N(kContactBytes), name, -1); }

bool load(const char* path, File& out) {
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(path);
    if (!res) {
        spdlog::error("MoveDef: {} : {} (offset {})", path, res.description(), res.offset);
        return false;
    }
    if (!doc.document_element()) {      // comments only (the panel's example.xml until its move is taken out of the comment)
        spdlog::info("MoveDef: {} holds only comments, skipped", path);
        return false;
    }
    pugi::xml_node move = doc.child("move");
    out = File{};
    out.source = path;
    if (!move) {
        // a library of shared commands: <commands character="pl006"><command name=...>...</command></commands>
        pugi::xml_node lib = doc.child("commands");
        if (!lib) {
            spdlog::error("MoveDef: {} has no <move> (or <commands>) root", path);
            return false;
        }
        out.commands_only = true;
        const char* lch = lib.attribute("character").as_string();
        if (ieq(lch, "pl000") || ieq(lch, "nero")) out.character = 0;
        else if (ieq(lch, "pl006") || ieq(lch, "dante")) out.character = 1;
        for (pugi::xml_node c : lib.children("command")) {
            Command cm;
            if (parse_command(path, c, cm)) out.shared_commands.push_back(cm);
        }
        spdlog::info("MoveDef: {} : {} shared command(s) for {}", path, out.shared_commands.size(), out.character == 0 ? "Nero" : out.character == 1 ? "Dante" : "?");
        return true;
    }
    out.name      = move.attribute("name").as_string();
    out.id        = (uint32_t)num(move.attribute("id"), 0);
    out.version   = (int)num(move.attribute("version"), 2);
    out.overlay   = bnum(move.attribute("overlay"), false) || move.child("routine");   // <routine native/> also marks one
    if (pugi::xml_attribute ids = move.attribute("ids")) {                             // "28 29" / "28,29": the first is `id`
        char buf[128]; strncpy_s(buf, ids.as_string(), sizeof buf - 1);
        char* ctx = nullptr;
        for (char* tok = strtok_s(buf, " ,", &ctx); tok; tok = strtok_s(nullptr, " ,", &ctx)) {
            const uint32_t v = (uint32_t)num(tok, 0);
            if (!v) continue;
            if (!out.id) out.id = v; else if (v != out.id) out.extra_ids.push_back(v);
        }
    }
    const char* ch = move.attribute("character").as_string();
    if (ieq(ch, "pl000") || ieq(ch, "nero")) out.character = 0;
    else if (ieq(ch, "pl006") || ieq(ch, "dante")) out.character = 1;

    if (pugi::xml_node l = move.child("lmt")) {
        out.lmt.bank = (int)num(l.attribute("bank"), -1);
        out.lmt.path = l.attribute("path").as_string();
    }

    if (pugi::xml_node tb = move.child("table")) {
        Table& t = out.table;
        t.present = true;
        t.attr    = (uint32_t)lookup(N(kAttrs), tb.attribute("attr").as_string(), 2);
        t.level   = (uint32_t)num(tb.attribute("level"), 7);
        t.start   = bnum(tb.attribute("start"), true);
        t.cancel  = bnum(tb.attribute("cancel"), false);
        t.repeat  = bnum(tb.attribute("repeat"), false);
        t.weapon  = (int32_t)num(tb.attribute("weapon"), -1);
        t.style   = (int32_t)num(tb.attribute("style"), -1);
        t.etc     = (uint32_t)num(tb.attribute("etc"), 0);
        t.air     = (uint32_t)lookup(N(kAir), tb.attribute("air").as_string(), 2);
        if (pugi::xml_attribute cw = tb.attribute("cancel_words")) {       // five words, comma separated
            unsigned w[5] = { 1, 0, 0, 0, 0x07000005 };
            sscanf_s(cw.as_string(), "%i,%i,%i,%i,%i", &w[0], &w[1], &w[2], &w[3], &w[4]);
            for (int i = 0; i < 5; ++i) t.cancel_words[i] = w[i];
        }
        if (pugi::xml_node in = tb.child("input")) t.command = in.attribute("command").as_string();
        t.explicit_words = tb.attribute("cancel_words") ? true : false;
        for (pugi::xml_node fo : tb.children("follows")) {
            Follows f;
            f.move   = fo.attribute("move").as_string();
            f.window = fo.attribute("window") ? cancel_category(fo.attribute("window").as_string()) : -1;
            f.any    = bnum(fo.attribute("or"), false);
            if (f.move.empty()) { spdlog::warn("MoveDef: {}: <follows> needs move=", path); continue; }
            t.follows.push_back(f);
        }
        for (pugi::xml_node wi : tb.children("window")) {
            Window w;
            w.category = cancel_category(wi.attribute("category").as_string());
            w.any      = bnum(wi.attribute("or"), false);
            if (w.category < 0) { spdlog::warn("MoveDef: {}: <window> needs a known category", path); continue; }
            t.windows.push_back(w);
        }
    }

    // the move's own <command> (the first), further ones are shared like a <commands> file's
    bool first = true;
    for (pugi::xml_node c : move.children("command")) {
        Command cm;
        if (!parse_command(path, c, cm)) continue;
        if (first) { out.command = cm; out.command.present = true; first = false; }
        else out.shared_commands.push_back(cm);
    }

    // stages: <stages><stage/>...</stages>, or <stage> elements straight under <move>
    pugi::xml_node stages = move.child("stages") ? move.child("stages") : move;
    for (pugi::xml_node st : stages.children("stage")) {
        Stage s;
        s.name        = st.attribute("name").as_string();
        s.motion      = parse_motion(st.attribute("motion").as_string());
        const int ctn = parse_motion(st.attribute("ctn").as_string());
        if (ctn >= 0) { s.ctn_bank = ctn >> 8; s.ctn_slot = ctn & 0xFF; }
        s.attr        = (uint32_t)lookup(N(kMotAttrs), st.attribute("attr").as_string(), 1);
        s.interp      = fnum(st.attribute("interp"), 4.0f);
        s.start       = fnum(st.attribute("start"), 0.0f);
        s.speed       = fnum(st.attribute("speed"), 1.0f);
        s.stop        = bnum(st.attribute("stop"), false);
        s.status_jump = ieq(st.attribute("status").as_string(), "jump");
        s.land        = st.attribute("land").as_string("exit");
        s.enemy_step  = bnum(st.attribute("enemy_step"), true);
        s.eat_press   = button_bits(st.attribute("eat_press").as_string());
        s.watchdog    = (int)num(st.attribute("watchdog"), 60);
        for (pugi::xml_node sl : st.children("slot")) {
            Slot slot;
            slot.dir   = dir_from_name(sl.attribute("dir").as_string());
            slot.block = (int)num(sl.attribute("block"), -1);
            slot.speed = fnum(sl.attribute("speed"), -1.0f);
            if (sl.attribute("rise"))        { slot.has_rise = true; slot.rise = fnum(sl.attribute("rise"), 0.0f); }
            if (sl.attribute("effect_roll")) { slot.has_roll = true; slot.roll = fnum(sl.attribute("effect_roll"), 0.0f); }
            slot.damage_type_b = (int)num(sl.attribute("damage_type_b"), -1);
            if (slot.dir < 0 || slot.block < 0) { spdlog::warn("MoveDef: {}: bad <slot dir='{}' block='{}'>", path, sl.attribute("dir").as_string(), sl.attribute("block").as_string()); continue; }
            s.slots.push_back(slot);
        }
        // cancel_frame="N" = everything opens at N; <cancel frame= kind= categories="C_JUMP C_ALL"/> for more
        if (pugi::xml_attribute cf = st.attribute("cancel_frame")) {
            const float f = cf.as_float(-1.0f);
            if (f >= 0.0f) { Cancel c; c.frame = f; c.mode = 0x10; s.cancels.push_back(c); }
        }
        for (pugi::xml_node cn : st.children("cancel")) {
            Cancel c;
            c.frame = fnum(cn.attribute("frame") ? cn.attribute("frame") : cn.attribute("from"), 0.0f);
            c.mode  = (uint8_t)lookup(N(kKinds), cn.attribute("kind").as_string(), 0x10);
            const char* cats = cn.attribute("categories") ? cn.attribute("categories").as_string() : cn.attribute("category").as_string();
            if (cats && *cats) {
                char buf[256]; strncpy_s(buf, cats, sizeof buf - 1);
                char* ctx = nullptr;
                for (char* tok = strtok_s(buf, " ,", &ctx); tok; tok = strtok_s(nullptr, " ,", &ctx)) {
                    const int cat = cancel_category(tok);
                    if (cat < 0) spdlog::warn("MoveDef: {}: unknown cancel category '{}'", path, tok);
                    else c.categories.push_back(cat);
                }
            }
            s.cancels.push_back(c);
        }
        std::stable_sort(s.cancels.begin(), s.cancels.end(), [](const Cancel& a, const Cancel& b) { return a.frame < b.frame; });
        for (pugi::xml_node tr : st.children("trigger")) {
            Trigger t;
            t.frame = fnum(tr.attribute("frame"), 0.0f);
            t.byte  = contact_byte(tr.attribute("byte").as_string("free0"));
            t.value = (uint8_t)num(tr.attribute("value"), 1);
            if (t.byte < 0) { spdlog::warn("MoveDef: {}: <trigger byte='{}'> unknown", path, tr.attribute("byte").as_string()); continue; }
            s.triggers.push_back(t);
        }
        std::stable_sort(s.triggers.begin(), s.triggers.end(), [](const Trigger& a, const Trigger& b) { return a.frame < b.frame; });
        for (pugi::xml_node el : st.children()) {           // the runtime timeline: <effect> / <se> / <kill_effect> / <finish_effect>
            FxEvent e;
            if (ieq(el.name(), "effect"))             e.kind = FxEvent::EFFECT;
            else if (ieq(el.name(), "se"))            e.kind = FxEvent::SE;
            else if (ieq(el.name(), "kill_effect"))   e.kind = FxEvent::KILL;
            else if (ieq(el.name(), "finish_effect")) e.kind = FxEvent::FINISH;
            else continue;
            e.frame  = fnum(el.attribute("frame"), 0.0f);
            e.repeat = ieq(el.attribute("repeat").as_string(), "loop");
            e.list   = (int)num(el.attribute("list"), -1);
            e.index  = (int)num(el.attribute("index"), 0);
            e.efl    = el.attribute("efl").as_string();
            e.joint  = el.attribute("joint").as_string("self");
            e.group  = (int32_t)num(el.attribute("group"), -1);
            const char* mat = el.attribute("material").as_string();
            e.material = ieq(mat, "ground") ? -2 : (int32_t)num(mat, -1);
            e.unit_line = (int)num(el.attribute("unitline"), 0x14);
            e.type   = (int)num(el.attribute("type"), 0);
            e.id     = (int)num(el.attribute("id"), -1);
            if (e.kind == FxEvent::EFFECT && e.list < 0 && e.efl.empty()) { spdlog::warn("MoveDef: {}: <effect> needs list= + index=, or efl=", path); continue; }
            if ((e.kind == FxEvent::KILL || e.kind == FxEvent::FINISH) && e.id < 0) { spdlog::warn("MoveDef: {}: <{}> needs id=", path, el.name()); continue; }
            s.fx.push_back(e);
        }
        std::stable_sort(s.fx.begin(), s.fx.end(), [](const FxEvent& a, const FxEvent& b) { return a.frame < b.frame; });
        for (pugi::xml_node nx : st.children("next")) {
            Next n;
            n.to = nx.attribute("to").as_string();
            if (n.to.rfind("move:", 0) == 0) n.to = n.to.substr(5);
            const char* on = nx.attribute("on").as_string();
            if (ieq(on, "motion_end")) n.motion_end = true;
            else if (ieq(on, "landing") || ieq(on, "land")) n.landing = true;
            else if (ieq(on, "always")) n.always = true;
            n.frame = fnum(nx.attribute("frame"), -1.0f);
            if (pugi::xml_attribute by = nx.attribute("byte")) {
                n.byte  = contact_byte(by.as_string());
                n.value = (int)num(nx.attribute("value"), 1);
                if (n.byte < 0) spdlog::warn("MoveDef: {}: <next byte='{}'> unknown", path, by.as_string());
            }
            n.button        = button_bits(nx.attribute("button").as_string());
            n.held          = bnum(nx.attribute("held"), false);
            n.idle          = (int)num(nx.attribute("idle"), -1);
            n.stick_neutral = ieq(nx.attribute("stick").as_string(), "neutral");
            if (n.to.empty()) { spdlog::warn("MoveDef: {}: <next> needs to=", path); continue; }
            if (n.frame < 0.0f && !n.motion_end && !n.landing && !n.always && n.byte < 0 && !n.button)
                spdlog::warn("MoveDef: {}: <next to='{}'> has no condition and never fires", path, n.to);
            s.nexts.push_back(n);
        }
        if (pugi::xml_node la = st.child("launch")) {
            Launch& L = s.launch;
            L.present    = true;
            L.frame      = fnum(la.attribute("frame"), 0.0f);
            L.direction  = la.attribute("direction").as_string("stick");
            L.speed      = fnum(la.attribute("speed"), 0.0f);
            L.rise       = fnum(la.attribute("rise"), 0.0f);
            L.gravity    = fnum(la.attribute("gravity"), 0.0f);
            L.max_fall   = fnum(la.attribute("max_fall"), 30.0f);
            L.side_angle = fnum(la.attribute("side_angle"), 50.0f);
            const char* fc = la.attribute("face").as_string("false");
            L.face       = ieq(fc, "all") ? 2 : la.attribute("face").as_bool(false) ? 1 : 0;
            if (!ieq(L.direction.c_str(), "stick") && !ieq(L.direction.c_str(), "facing") && !ieq(L.direction.c_str(), "vertical"))
                spdlog::warn("MoveDef: {}: <launch direction='{}'> unknown (stick, facing, vertical); taken as facing", path, L.direction);
            for (pugi::xml_node sh : la.children("shell")) {     // spawned at the launch, along its direction
                Shell def;
                parse_shell(sh, out.character, def);
                def.in_launch = true;
                def.frame = L.frame;
                s.shells.push_back(def);
            }
            if (pugi::xml_node ef = la.child("effect")) {
                L.fx.effect = true;
                L.fx.list   = (int)num(ef.attribute("list"), 0);
                L.fx.index  = (int)num(ef.attribute("index"), 0);
                L.fx.tilt   = fnum(ef.attribute("tilt"), 0.0f);
                L.fx.roll   = fnum(ef.attribute("roll"), 0.0f);
            }
            if (pugi::xml_node se = la.child("se")) {
                L.fx.se    = true;
                L.fx.se_id = (int)num(se.attribute("id"), 0);
            }
        }
        for (pugi::xml_node sh : st.children("shell")) {         // spawned at its frame of the stage's motion
            Shell def;
            parse_shell(sh, out.character, def);
            def.frame = fnum(sh.attribute("frame"), 0.0f);
            s.shells.push_back(def);
        }
        if (s.shells.size() > 8) {
            spdlog::warn("MoveDef: {}: {} <shell>s in one stage, 8 are used", path, s.shells.size());
            s.shells.resize(8);
        }
        if (pugi::xml_node aj = st.child("air_jump")) {
            s.air_jump.cost              = (int)num(aj.attribute("cost"), 0);
            s.air_jump.required          = bnum(aj.attribute("required"), false);
            s.air_jump.suppress_air_hike = bnum(aj.attribute("suppress_air_hike"), false);
            s.air_jump.per_air           = (int)num(aj.attribute("per_air"), 0);
        }
        out.stages.push_back(s);
    }

    spdlog::info("MoveDef: {} '{}' {} id {:#x}: table {}, command {}, lmt bank {} '{}', {} stage(s)", path, out.name,
                 out.character == 0 ? "Nero" : out.character == 1 ? "Dante" : "?", out.id,
                 out.table.present ? "yes" : "no", out.command.present ? out.command.name : "none", out.lmt.bank, out.lmt.path, out.stages.size());
    return true;
}

} // namespace xmlmove
