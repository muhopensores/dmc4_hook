#ifndef HACKLIB_TIMER_H
#define HACKLIB_TIMER_H

#include <chrono>


namespace hl
{
class Timer
{
    typedef std::chrono::high_resolution_clock Clock;

public:
    void reset() { m_timestamp = Clock::now(); }

    template <typename T = float>
    T diff() const
    {
        std::chrono::duration<T> fs = Clock::now() - m_timestamp;
        return fs.count();
    }

private:
    Clock::time_point m_timestamp = Clock::now();
};
}

#endif
