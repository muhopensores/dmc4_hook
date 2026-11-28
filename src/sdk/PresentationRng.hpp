#pragma once
// presentation_rng - a random stream of its own for code that only changes what is seen or heard.
//
// The game has one random stream (MtRandom, sdk/MtRandom.hpp) for everything: enemy AI and the coat's turbulence alike.
// Under netplay a draw that one machine makes and the other doesn't (an effect, a sound, a cosmetic difference) moves
// the stream for every gameplay draw after it, and the games split. Code that only changes what is seen or heard runs
// with the stream swapped for this one (the 625 words exchanged in place: 60 of the game's draws address the block
// directly), so its draws never touch the gameplay stream. The swap is on only while netplay or the rollback probe runs
// the ticks (set_enabled); the stream is this machine's own and is never rewound, except where a call seeds it first
// (enter_seeded): what an effect generator draws decides how long the effect lives, and some units live as long as their
// effect (the em030 shells), so those draws have to be the same on every machine.
//
// While enabled, the ticks run their units on one thread (CoopOnline's MainThreadJobs: sMain's job count 1), the thread
// set_enabled ran on, so there is one stream at a time; calls from any other thread (the game's job threads, which run
// the chains when netplay is off) pass through untouched. enter / leave nest and touch no SSE or x87 state: they are
// called from thunks around the game's own functions (xmm arguments, ST0 results).

#include <cstdint>

namespace presentation_rng {

void set_enabled(bool on);   // on the thread that runs the ticks

void enter();
// As enter, but the outermost one first seeds the stream from `seed`: code whose draws must come out the same on every
// machine for the same call (an effect generator's parameters: a shell that dies with its effect lives as long as they
// say), whatever else this machine drew from the stream before.
void enter_seeded(uint32_t seed);
void leave();

struct Scope {
    Scope() { enter(); }
    ~Scope() { leave(); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};

} // namespace presentation_rng
