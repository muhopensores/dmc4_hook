#pragma once

#include <functional>
#include "utility/Timer.hpp"
#include "utility/String.hpp"
#include <random>

namespace utility {
    class Config;
}

class Mutator {

    using TimeExpiredFunction = std::function<void()>;
    using InitFunction = std::function<void()>;

public:

    Mutator(const std::string& name) : m_name(name) {};

    //size_t m_handle;
    std::string m_name;
    std::string m_description;

    // TODO(): enable mutators in ui or some shit like that
    bool m_enabled = false;
    bool* m_extra_arg = nullptr ; // TODO(): std::variant crazyness maybe?
    // TODO(): default value, we need to have a vector of weights for
    // std::discrete_distribution, so MutatorRegistry has one,
    // this is for default values when users config is empty
    // to follow Mutator interface we kept one here.
    int32_t m_weight{6};

    InitFunction m_init = []{};
    utility::Timer* m_timer{ nullptr };
    std::vector<size_t> m_aliases {};

    // below are hacks because i dont want to target C++20 with designated initializer lists support

    Mutator& description(const std::string&& description);
    Mutator& on_init(const InitFunction &func);
    Mutator& set_timer(float seconds, TimeExpiredFunction on_timer_expired);
    Mutator& alias(const std::string& alias);
    Mutator& weight(int32_t weight);
    Mutator& special_arg(bool* arg);
};

struct MutatorRegistry {
    
    static MutatorRegistry& inst() {
        static MutatorRegistry instance;
        return instance;
    }

    static Mutator& define(const std::string& name);
    
    void activate_mod(Mutator* m);
    void activate_mod(size_t hash);
    void activate_mod(const std::string& name);

    void update(fmilliseconds dt);
    void save_config(utility::Config& cfg);
    void load_config(const utility::Config& cfg);
    
    Mutator* get_ptr(size_t hash);
    size_t get_index(size_t hash);

    std::vector<Mutator*> m_mutators;
    std::vector<int32_t>  m_weights; // for std::discrete distribution
    std::vector<size_t>   m_hashes;

private:
    MutatorRegistry();
};

// TODO(): NAMESPACE!!??
void mutator_draw_mod_table(MutatorRegistry& registry);
float mutator_is_mod_active(Mutator* mutator);