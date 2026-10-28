#pragma once

#include "../../mod.hpp"
#include <string>
#include <vector>

// ModelExtension: extends the game's models from lists of the user's, on whichever models the lists name, when those
// models appear: physics chains (.clt: coats, hair, tails) through runtime uCnsChain units (sdk/CnsChain.hpp; RE note
// chain_cnschain.md section 6) and extra models (hats, coats, accessories) attached to them (sdk/ModelAttach.hpp; RE note
// model_attachments.md). The game's own chains and parts are hard-coded per owner and their file names follow no scheme,
// so the lists say everything:
//
//   nativePC\model_ext\*.xml, game paths without extension (models are .mod paths as the models were loaded, e.g.
//   model\game\pl000\pl000_03 = Nero's coat, model\game\pl000\pl000 = his body):
//     <extensions>
//       <attach path="model\game\pl000\my_jacket" to="model\game\pl000\pl000"/>                  skinned to the body
//       <attach path="model\game\pl000\my_hat" to="model\game\pl000\pl000" joint="4" pos="0 5 0" rot="0 90 0" scale="1"/>
//       <model path="model\game\pl000\my_jacket" collision_model="model\game\pl000\pl000">       chains on the jacket
//         <chain clt="model\game\pl000\my_jacket_tail" col="model\game\pl000\pl000_03"/>
//       </model>
//     </extensions>
//   <attach>: without joint= (or mode="skin") every joint of the extra model follows the owner's joint with the same number
//   (the body's bind pose needed); with joint= (an owner joint NUMBER; mode="rigid") it rides that joint, offset by pos (cm),
//   rot (degrees about X, then Y, then Z) and scale (one number or three). line= overrides its move line (29). An attached
//   model is a model like any other: <model> entries with its path put chains on it (its extra joints), <attach> to= its
//   path nests attachments.
//   <chain>: col (the .col) and collision_model (the model its shapes sit on) are optional, collision needs both; a
//   <chain>'s collision_model overrides its <model>'s. The chain's joint numbers are the model's joints; the collision
//   shape's sit on the collision model's.
//
// Event-driven: uModel::setModel 0x9E6D70 (every model gets its .mod through it, sub-models such as the players' coat and
// face models too) queues the model at its entry, and the next frame (on the main thread) reads its path and spawns the
// chains its lists give it, on the line below the model's (at most 12; 12 for a sub-model, which is on no line), and its
// attachments (line 29, updated in the moveAfter pass). The uModel destructor body 0x9E6AB0 (every derived destructor calls
// it, joints still intact at its entry) takes the model's chains and attachments off and forgets it. A room change kills
// the gameplay lines (12 and 29 included) while some models live on (the player, his coat): every 10 frames the chains out
// are checked and a killed one is spawned again for a model still alive; a killed attachment re-queues its owner from its
// destructor. ~cCnsChain 0x42C7A0 is hooked too: it clears the constraint of every joint of its model, which would drop
// the constraints that claim a joint once (Nero's coat joint 0 cCnsMatrix); one of our chains lets go of its own joints
// and of its model first, so the sweep finds nothing. Don't list joints another constraint drives (Nero's coat joint 0,
// the vanilla coat chains' joints): a joint holds one constraint and the chain takes it every frame.

namespace model_extension {

struct ListEntry {
    std::string model, chain, col, collision_model;   // lowercase, backslashes
    std::string source;                               // file, model, number: for the readout
};

extern bool mod_enabled;
extern char folder[128];

void reload();     // reads the lists again; every chain comes off and the known models get theirs again next frame
void write_example();   // example.xml in the lists folder (all commented out), unless it exists

void __stdcall on_set_model(uintptr_t model);    // the hooks' bodies
void __stdcall on_model_dtor(uintptr_t model);
void detour_set_model();
void detour_model_dtor();
void __stdcall on_chain_dtor(uintptr_t cns);
void detour_chain_dtor();
extern uintptr_t jmp_ret_set_model;   // 0x9E6D76
extern uintptr_t jmp_ret_model_dtor;  // 0x9E6AB6
extern uintptr_t jmp_ret_chain_dtor;  // 0x42C7A6

} // namespace model_extension

class ModelExtension : public Mod {
public:
    ModelExtension() = default;
    std::string get_mod_name() override { return "ModelExtension"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook_set_model, hook_model_dtor, hook_chain_dtor;
};
