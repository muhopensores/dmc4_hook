#include "PluginMenus.hpp"
#include "PluginHost.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../../sdk/PauseMenu.hpp"
#include "../../sdk/MissionMenu.hpp"
#include <cstring>
#include <mutex>

namespace plugins {
namespace {

std::mutex g_mutex;
dmc4_plugin* g_owner = nullptr;   // the plugin whose pause row is set
dmc4_plugin* g_mission_owner = nullptr;   // the plugin whose Mission Menu row is set
dmc4_mission_row g_mission;   // its callback (copied)

void c_mission(void* p) { auto* r = static_cast<dmc4_mission_row*>(p); r->decide(r->user); }

// one callback call with its arguments, through run_guarded (which takes a plain fn(user))
struct Call {
    const dmc4_list_menu* menu;
    int row = 0, step = 0, result = 0;
    char label[96] = {}, value[96] = {};
};
dmc4_list_menu g_menu;   // the plugin's callbacks (copied)

bool owns(dmc4_plugin* self) {   // a stopped plugin's open menu goes on with empty rows
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_owner == self;
}

void c_count(void* p) { auto* c = static_cast<Call*>(p); c->result = c->menu->row_count(c->menu->user); }
void c_text(void* p) {
    auto* c = static_cast<Call*>(p);
    c->menu->row_text(c->menu->user, c->row, c->label, sizeof c->label, c->value, sizeof c->value);
    c->label[sizeof c->label - 1] = c->value[sizeof c->value - 1] = 0;
}
void c_change(void* p) { auto* c = static_cast<Call*>(p); c->menu->change(c->menu->user, c->row, c->step); }
void c_decide(void* p) { auto* c = static_cast<Call*>(p); c->menu->decide(c->menu->user, c->row); }
void c_page(void* p) { auto* c = static_cast<Call*>(p); c->result = c->menu->page(c->menu->user); }
void c_title(void* p) { auto* c = static_cast<Call*>(p); c->result = c->menu->title_image(c->menu->user); }
void c_back(void* p) { auto* c = static_cast<Call*>(p); c->result = c->menu->back(c->menu->user); }
void c_images(void* p) {
    auto* c = static_cast<Call*>(p);
    int label = 0, value = 0;
    c->menu->row_images(c->menu->user, c->row, &label, &value);
    c->result = label;
    c->step = value;
}

int __cdecl api_pause_row(dmc4_plugin* self, const dmc4_pause_row* row) {
    if (!self || !row || row->size < sizeof(dmc4_pause_row) || !row->label || !row->menu.row_count || !row->menu.row_text)
        return 0;
    if (!pause_menu::install()) {
        plugin_log(self, DMC4_LOG_ERROR, "menus->pause_row: the pause menu isn't the expected code");
        return 0;
    }
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_owner = self;
        g_menu = row->menu;
    }
    pause_menu::Row r;
    r.room = row->room;
    r.label = row->label;
    r.description = row->description ? row->description : "";
    r.label_image = row->label_image - 1;
    r.label_image_width = row->label_image_width;
    r.title_image = row->title_image - 1;
    r.menu.title = row->menu.title ? row->menu.title : "";
    r.menu.rows = [self]() {
        Call c{&g_menu};
        if (!owns(self)) return 0;
        return run_guarded(self, c_count, &c, "a list menu's row_count") ? c.result : 0;
    };
    r.menu.text = [self](int i, std::string& label, std::string& value) {
        Call c{&g_menu};
        c.row = i;
        if (owns(self) && run_guarded(self, c_text, &c, "a list menu's row_text")) {
            label = c.label;
            value = c.value;
        }
    };
    if (row->menu.change)
        r.menu.change = [self](int i, int step) {
            Call c{&g_menu};
            c.row = i;
            c.step = step;
            if (owns(self)) run_guarded(self, c_change, &c, "a list menu's change");
        };
    if (row->menu.decide)
        r.menu.decide = [self](int i) {
            Call c{&g_menu};
            c.row = i;
            if (owns(self)) run_guarded(self, c_decide, &c, "a list menu's decide");
        };
    if (row->menu.page)
        r.menu.page = [self]() {
            Call c{&g_menu};
            return owns(self) && run_guarded(self, c_page, &c, "a list menu's page") ? c.result : 0;
        };
    if (row->menu.title_image)
        r.menu.title_image = [self]() {
            Call c{&g_menu};
            return owns(self) && run_guarded(self, c_title, &c, "a list menu's title_image") ? c.result - 1 : -1;
        };
    if (row->menu.row_images)
        r.menu.images = [self](int i, int& label, int& value) {
            Call c{&g_menu};
            c.row = i;
            if (owns(self) && run_guarded(self, c_images, &c, "a list menu's row_images")) {
                label = c.result - 1;
                value = c.step - 1;
            }
        };
    if (row->menu.back)
        r.menu.back = [self]() {
            Call c{&g_menu};
            return owns(self) && run_guarded(self, c_back, &c, "a list menu's back") && c.result != 0;
        };
    pause_menu::set_row(r);
    return 1;
}

void __cdecl api_clear_pause_row(dmc4_plugin* self) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_owner != self) return;
    g_owner = nullptr;
    pause_menu::clear_row();
}

int __cdecl api_mission_row(dmc4_plugin* self, const dmc4_mission_row* row) {
    if (!self || !row || row->size < sizeof(dmc4_mission_row) || !row->label || !row->decide) return 0;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_mission_owner = self;
        g_mission = *row;
    }
    mission_menu::Row r;
    r.label = row->label;
    r.description = row->description ? row->description : "";
    r.label_image = row->label_image - 1;
    r.label_image_selected = row->label_image_selected - 1;
    r.decide = [self]() {
        dmc4_mission_row copy;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (g_mission_owner != self) return;
            copy = g_mission;
        }
        run_guarded(self, c_mission, &copy, "a Mission Menu row's decide");
    };
    if (!mission_menu::set_row(r)) {
        plugin_log(self, DMC4_LOG_ERROR, "menus->mission_row: the Mission Menu isn't the expected code");
        return 0;
    }
    return 1;
}

void __cdecl api_clear_mission_row(dmc4_plugin* self) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_mission_owner != self) return;
    g_mission_owner = nullptr;
    mission_menu::clear_row();
}
int __cdecl api_menu_open() { return pause_menu::menu_open() ? 1 : 0; }

const dmc4_menus_api g_menus = {
    sizeof(dmc4_menus_api),
    api_pause_row,
    api_clear_pause_row,
    api_menu_open,
    api_mission_row,
    api_clear_mission_row,
};

} // namespace

const dmc4_menus_api* menus_api() { return &g_menus; }

void release_menus(dmc4_plugin* self) {
    api_clear_pause_row(self);
    api_clear_mission_row(self);
}

} // namespace plugins
