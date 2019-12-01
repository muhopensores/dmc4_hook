#ifndef HACKLIB_RNG_H
#define HACKLIB_RNG_H

#include <random>


namespace hl
{
class Rng
{
public:
    Rng() : m_rng(std::random_device()()) {}

    template <typename T>
    T nextInt(T min, T max)
    {
        std::uniform_int_distribution<T> distr(min, max);
        return distr(m_rng);
    }

    template <typename T>
    T nextReal(T min, T max)
    {
        std::uniform_real_distribution<T> distr(min, max);
        return distr(m_rng);
    }

private:
    std::mt19937 m_rng;
};
}

#endif