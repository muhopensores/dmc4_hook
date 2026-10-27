#include "CnsChain.hpp"
#include "Devil4.hpp"

namespace cns_chain {

namespace {
constexpr uintptr_t fn_newInstance      = 0x004FC420;   // uCnsChain DTI newInstance: no arguments, eax = the unit (0x10E0) or 0
constexpr uintptr_t fn_setCltRes        = 0x0042D120;   // cCnsChain::setCltRes(this@ecx, rsrc), retn 4: addRefs, copies the .clt's parameters
constexpr uintptr_t fn_setCollisionShape = 0x004393D0;  // cCnsChain collision-shape setter, same convention, addRefs
constexpr uintptr_t fn_setEnable        = 0x004FC520;   // uCnsChain::setEnable(enable@al, this@edx), retn: releases its joints when off
constexpr uintptr_t kDtiCnsChainRes     = 0x00E57538;   // rCnsChain::DTI
constexpr uintptr_t kDtiCollisionShape  = 0x00E575F8;   // rCollisionShape::DTI
// uCnsChain fields (the cCnsChain is at +0x20)
constexpr uintptr_t kChain = 0x20, kModel = 0xFD4, kCollisionModel = 0xFD8, kResetState = 0xFF0;

uintptr_t new_unit() {
    uintptr_t unit = 0;
    _asm {
        pushad
        call fn_newInstance
        mov unit, eax
        popad
    }
    return unit;
}

uintptr_t load(uintptr_t dti, const char* path) {
    if (!path || !*path) return 0;
    return (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)dti, (char*)path, 1);
}

void set_resource(uintptr_t setter, uintptr_t chain, uintptr_t rsrc) {
    ((void(__fastcall*)(uintptr_t, int, uintptr_t))setter)(chain, 0, rsrc);   // __thiscall, callee pops its argument
}
} // namespace

uintptr_t spawn(uintptr_t model, const char* clt_path, const char* col_path, uintptr_t collision_model, int line) {
    if (!model || line < 0 || line > 31) return 0;
    // the resources first: a unit that never gets registered would have to be destroyed by hand
    const uintptr_t clt = load(kDtiCnsChainRes, clt_path);
    if (!clt) return 0;
    const uintptr_t col = (col_path && collision_model) ? load(kDtiCollisionShape, col_path) : 0;
    const uintptr_t unit = new_unit();
    if (!unit) {
        devil4_sdk::release_resource((cResource*)clt);
        if (col) devil4_sdk::release_resource((cResource*)col);
        return 0;
    }
    set_resource(fn_setCltRes, unit + kChain, clt);            // the chain keeps its own reference
    devil4_sdk::release_resource((cResource*)clt);              // ours from sResource::create goes
    if (col) {
        set_resource(fn_setCollisionShape, unit + kChain, col);
        devil4_sdk::release_resource((cResource*)col);
    }
    *(uintptr_t*)(unit + kModel) = model;
    *(uintptr_t*)(unit + kCollisionModel) = col ? collision_model : 0;
    *(uint8_t*)(unit + kResetState) = 1;     // the first two solves copy the animated pose (else it starts from identity)
    devil4_sdk::sUnit_spawn((void*)unit, line);
    return unit;
}

void detach(uintptr_t chain, bool model_alive) {
    if (!chain) return;
    if (model_alive && model_of(chain)) {   // setEnable(false) clears joint+0xC on the chain's own mBone joints only
        _asm {
            pushad
            xor eax, eax                    // al = 0: off
            mov edx, chain
            call fn_setEnable
            popad
        }
    }
    *(uintptr_t*)(chain + kModel) = 0;
    *(uintptr_t*)(chain + kCollisionModel) = 0;
}

void despawn(uintptr_t chain, bool model_alive) {
    if (!chain) return;
    detach(chain, model_alive);
    uactor_sdk::despawn((void*)chain);      // state 3: sUnit unlinks and deletes it
}

uintptr_t model_of(uintptr_t chain) { return chain ? *(uintptr_t*)(chain + kModel) : 0; }

} // namespace cns_chain
