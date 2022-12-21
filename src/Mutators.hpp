#pragma once

#include <functional>
#include "utility/Timer.hpp"
#include "utility/String.hpp"

class Mutator {

    using TimeExpiredFunction = std::function<void()>;
    using InitFunction = std::function<void()>;

public:

    Mutator(const std::string& name) : m_name(name) {};

    //size_t m_handle;
    std::string m_name;
    std::string m_description;

    // TODO(): enable mutators in ui or some shit like that
    bool m_enabled{ false };
    bool* m_extra_arg{ nullptr }; // TODO(): std::variant crazyness maybe?
    //bool m_is_active;

    InitFunction m_init = []{};
    utility::Timer* m_timer{ nullptr };
    std::vector<size_t> m_aliases {};

    // below are hacks because i dont want to get a C++20 compiler with designated initializer lists support

    Mutator& description(const std::string&& description);
    Mutator& on_init(const InitFunction &func);
    Mutator& set_timer(float seconds, TimeExpiredFunction on_timer_expired);
    Mutator& alias(const std::string& alias);
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
    
    Mutator* get_ptr(size_t hash);

    std::vector<Mutator*> m_mutators;
    std::vector<size_t>   m_hashes;

private:
    MutatorRegistry();
};