// frame_pacer.h - fixed-rate frame pacing for a fixed-step sMain::move replacement.
// See RE/sMain_move_timing.md (option B).
//
// Win32 only. Header-only, C++11. Links against kernel32 and nothing else.
//
// Call wait_for_next_frame_deadline() once per presented frame, from the game's main
// thread only. Do not call it for re-simulated ticks.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002  // Windows 10 1803+
#endif

namespace frame_pacer {

// After a stall longer than this many frames the schedule restarts from "now".
// Shorter delays are caught up by running the following frames early, so the
// long-run rate stays at exactly `fps`.
const int kMaxCatchUpFrames = 4;

struct State {
    int64_t freq;        // QueryPerformanceCounter ticks per second
    int64_t deadline;    // counter value at which the current frame ends; 0 = not started
    int64_t spin_ticks;  // final part of each wait that is spun instead of slept
    HANDLE  timer;       // high-resolution waitable timer; null = spin the whole wait
};

inline State& state()
{
    static State s = {0, 0, 0, nullptr};
    return s;
}

inline int64_t now()
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

inline void init(State& s)
{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    s.freq = f.QuadPart;

    // The high-resolution timer wakes within about 1 ms of the request regardless of the
    // system timer tick. The last 2 ms are spun to absorb that. Without the timer
    // (Windows older than 10 1803) the whole wait is spun: accurate, but uses one core.
    s.spin_ticks = s.freq / 500;
    s.timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                     TIMER_ALL_ACCESS);
}

// Forget the schedule. The next wait returns immediately and starts a new one.
// Use after loading screens or any pause that should not be caught up.
inline void reset()
{
    state().deadline = 0;
}

} // namespace frame_pacer

// Blocks until the current frame's deadline, then schedules the next one 1/fps later.
// The first call (and the first after reset()) returns immediately.
inline void wait_for_next_frame_deadline(double fps = 60.0)
{
    using namespace frame_pacer;
    State& s = state();
    if (!s.freq)
        init(s);

    // the game's rate can read 0 before its first frame-rate request; a period from it never ends
    if (!(fps >= 10.0) || fps > 1000.0)
        fps = 60.0;
    const int64_t period = (int64_t)((double)s.freq / fps + 0.5);
    int64_t t = now();

    // a stall restarts the schedule, and so does a deadline further ahead than the catch-up window
    if (!s.deadline || t - s.deadline > period * kMaxCatchUpFrames || s.deadline - t > period * kMaxCatchUpFrames) {
        s.deadline = t + period;
        return;
    }

    // Sleep until spin_ticks before the deadline, then spin the rest.
    while (s.timer && s.deadline - t > s.spin_ticks) {
        LARGE_INTEGER due;  // negative = relative, in 100 ns units
        due.QuadPart = -((s.deadline - t - s.spin_ticks) * 10000000 / s.freq);
        if (due.QuadPart == 0 || !SetWaitableTimer(s.timer, &due, 0, nullptr, nullptr, FALSE))
            break;
        WaitForSingleObject(s.timer, INFINITE);
        t = now();
    }
    while (now() < s.deadline)
        YieldProcessor();

    s.deadline += period;
}
