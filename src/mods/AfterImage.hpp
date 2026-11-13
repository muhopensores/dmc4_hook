#pragma once
#include "../mod.hpp"

// AfterImage: copies of the player's pose left behind that glow at their edges and fade, like Ninja Gaiden's after
// images. Each copy is a unit (CustomUnit) carrying the player's four models (body as itself, head / hair / coat as
// models of its own, attached to its joints the way the player's are), posed by copying the player's joint matrices.
//
// ModelExtension: the models its <attach>es hang on the player, his parts or those attachments are copied too (as many as
// there are: a copy grows a model per attachment, kept for reuse), posed by their own joint matrices; models a <hide> keeps from being drawn (a replaced face or coat) are left out. The
// copies are excluded from its lists (model_extension::exclude): their .mod paths are the player's, and the lists would
// otherwise hang chains and attachments on every copy and hide its parts.
//
// Pool: the copies are made once (up to pool_size) and reused; a spent copy stays alive, hidden (mTransparency 0, which
// uDevil4Model::draw 0x522DC0 skips), until it is needed again. Its models are set up again only when the player's
// rModel changed (Dante's Devil Trigger, another costume). When every copy is in use the oldest is taken.
//
// Glow: the copies' materials go through sdk/RimGlow (the edge glow and how it works); the cube holds rim_color.

namespace after_image {

constexpr int kPoolMax = 16;

extern bool  trail;                   // copies are left behind every player while he moves fast (each his own timer)
// the trail's gates (any one on and open leaves copies)
extern bool  gate_inertia;            // his mMoveVel (the game's move speed, inertia) above trail_speed
extern float trail_speed;             // cm per frame
extern bool  gate_velocity;           // his real movement (position change, any direction) above velocity_speed
extern float velocity_speed;          // cm per frame
extern bool  gate_dodge;              // while an evasive move's uPlCmnShl000 (the game's dodge detector) is out for him
extern bool  dodge_jumps;             // plain jumps count as dodges too (they spawn the detector as well)
extern bool  gate_mods;               // while a mod's move asks for the trail (trail_request)
extern float trail_interval;          // frames between copies
extern float hold_frames;             // frames a copy stays at full strength
extern float fade_frames;             // frames it then fades over
extern int   pool_size;               // copies kept for reuse (1 - kPoolMax)

extern bool  glow;                    // the edge glow; off: plain see-through copies
extern float rim_color[3];            // the glow's colour
extern float rim_power;               // its brightness
extern float rim_reach;               // how far in from the silhouette it reaches (the facing cosine where it ends)
extern float opacity;                 // the copy's strength before it fades
extern bool  keep_mask;               // the glow keeps the model's specular masks (detail) instead of an even colour
extern bool  through_walls;           // the zthrough bit: drawn over what is in front of it

extern bool  use_model_paths;         // replacement models for the copies (e.g. pl006_afimg), when they load
extern char  model_paths[2][4][128];  // per character (0 Dante, 1 Nero): body, head, hair, coat ("" = the player's own)

void request_spawn();                 // one copy of player 1 at his next tick (any thread)
bool spawn(uintptr_t player);         // one copy of this player now (his tick only); false when it couldn't be made
void trail_request(uintptr_t player); // a mod's move wants the trail: call it on every tick it does (the next tick reads it)

} // namespace after_image

class AfterImage : public Mod {
public:
    AfterImage() = default;
    std::string get_mod_name() override { return "AfterImage"; };
    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    std::unique_ptr<FunctionHook> hook_cmd;
};
