#pragma once
// host_rules - settings that change the simulation, which every machine of an online co-op session plays by the host's
// values of.
//
// A mod registers each such variable once, at start-up (add). Offline nothing changes. With the online lobby open:
//  - the host's GUI edits ui(x), a copy: each change becomes a new version, which the lobby sends to the other machines.
//    Once every machine has it, the host's input names it, and each simulated tick (tick) every machine puts the
//    version its host input names into the variables: they all switch on the same frame, re-simulated frames included.
//    Between sessions (no stage running) the variables take the host's values at once;
//  - the other machines (guests) show the host's values (from_host(): draw those controls disabled), and their cfg keeps
//    their own (own(x) in on_config_save); their own values come back when they leave.
// Plain data only, the same layout on every machine (the lobby connects the same build only).
#include <cstddef>
#include <cstdint>
#include <vector>

namespace host_rules {

int add(const char* name, void* data, size_t bytes);   // > 0 id
template <class T> int add(const char* name, T& v) { return add(name, &v, sizeof v); }

// What the GUI edits: the host's copy while the lobby is open, else the variable itself.
void* ui_ptr(void* data);
template <class T> T& ui(T& v) { return *static_cast<T*>(ui_ptr(&v)); }
// What the cfg keeps: a guest's own value while it plays by the host's, the host's latest edit, else the variable.
const void* own_ptr(const void* data);
template <class T> T own(const T& v) { return *static_cast<const T*>(own_ptr(&v)); }
bool from_host();   // a guest with the lobby open: the registered values are the host's

// ---- the lobby and netplay -----------------------------------------------------------------------------------------
enum class Role : uint8_t { None, Host, Guest };
void set_role(Role role);      // the lobby opened as the host / a guest, or closed (a guest's own values come back)
void set_session(bool running);   // a GekkoNet session runs: the values change only through tick()
constexpr size_t kMaxBytes = 1024;   // every registered value together
size_t bytes();

// host
void host_frame();                                // every frame: an edit becomes the next version
uint8_t latest();                                 // the newest version (1-255; 0 none)
bool blob(uint8_t version, std::vector<uint8_t>& out);
void set_in_force(uint8_t version);               // every guest has it: the host's input names it from now
uint8_t in_force();                               // what the host's input carries

// guests
void receive(uint8_t version, const uint8_t* data, size_t size);
uint8_t newest_received();                        // for the guest's hello: the host waits for every guest

// every machine, each simulated tick of an online session: the version the host's input names into the variables
void tick(uint8_t version);

} // namespace host_rules
