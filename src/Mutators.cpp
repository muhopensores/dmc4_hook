#include "Mutators.hpp"
#include <cassert>
#include "sdk/Devil4.hpp"

static constexpr size_t NUM_MUTATORS_RESERVE = 128;

MutatorRegistry* g_mutator_registry{ nullptr };

MutatorRegistry::MutatorRegistry() {
    g_mutator_registry = this;

    m_hashes  .reserve(NUM_MUTATORS_RESERVE);
    m_mutators.reserve(NUM_MUTATORS_RESERVE);
}

Mutator& MutatorRegistry::define(const std::string& name) {
    assert(name.length() < 64);

    if (!g_mutator_registry) { MutatorRegistry::inst(); }
    auto* mod = new Mutator(name);

    g_mutator_registry->m_mutators.push_back(mod);
    g_mutator_registry->m_hashes.emplace_back(std::hash<std::string>{}(name));

    return *mod;
}

Mutator* MutatorRegistry::get_ptr(size_t hash) {
    auto itr = std::find(m_hashes.begin(), m_hashes.end(), hash);
    if (itr != m_hashes.cend()) {
        return m_mutators[std::distance(m_hashes.begin(), itr)];
    }
    return nullptr;
}

void MutatorRegistry::activate_mod(Mutator* m) {
    if (devil4_sdk::is_not_in_gameplay()) { return; }
    m->m_init();
    if (m->m_timer) {
        m->m_timer->start();
    }
}

void MutatorRegistry::activate_mod(size_t hash) {
    /* assert(g_mutator_registry);*/
    Mutator* m = get_ptr(hash);
    if (!m) { return; }
    activate_mod(m);
}

void MutatorRegistry::activate_mod(const std::string& name) {
    /*assert(g_mutator_registry);*/
    auto hash = std::hash<std::string>{}(name);
    activate_mod(hash);
}

void MutatorRegistry::update(const fmilliseconds dt) {
    for (auto& mutator : m_mutators) {
        if (mutator->m_timer) {
            mutator->m_timer->tick(dt);
        }
    }
}

Mutator& Mutator::description(const std::string&& description) {
    m_description = std::move(description);
    return *this;
}

Mutator& Mutator::on_init(const InitFunction &func) {
    m_init = func;
    return *this;
}

Mutator& Mutator::set_timer(float seconds, TimeExpiredFunction on_timer_expired) {
    m_timer = new utility::Timer(seconds, on_timer_expired);
    return *this;
}

Mutator& Mutator::alias(const std::string& alias)
{
    m_aliases.emplace_back(std::hash<std::string>{}(alias));
    return *this;
}

