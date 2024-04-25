#include "Mutators.hpp"
#include <cassert>
#include "sdk/Devil4.hpp"
#include "GuiFunctions.hpp"

static constexpr size_t NUM_MUTATORS_RESERVE = 128;

MutatorRegistry* g_mutator_registry{ nullptr };

MutatorRegistry::MutatorRegistry() {
    g_mutator_registry = this;

    m_hashes  .reserve(NUM_MUTATORS_RESERVE);
    m_mutators.reserve(NUM_MUTATORS_RESERVE);
    m_weights .reserve(NUM_MUTATORS_RESERVE);
}

Mutator& MutatorRegistry::define(const std::string& name) {
    assert(name.length() < 64);

    if (!g_mutator_registry) { MutatorRegistry::inst(); }
    auto name_lower = utility::to_lower(name);
    auto* mod = new Mutator(name);
    mod->m_enabled = true;

    g_mutator_registry->m_mutators.push_back(mod);
    g_mutator_registry->m_hashes.emplace_back(std::hash<std::string>{}(name_lower));
    g_mutator_registry->m_weights.emplace_back(g_mutator_registry->m_mutators.size()/100);
    return *mod;
}

size_t MutatorRegistry::get_index(size_t hash)
{
    auto itr = std::find(m_hashes.begin(), m_hashes.end(), hash);
    if (itr != m_hashes.cend()) {
        return std::distance(m_hashes.begin(), itr);
    }
    assert(false && "MutatorRegistry::get_index UNREACHABLE");
    return -1;
}

Mutator* MutatorRegistry::get_ptr(size_t hash) {
    
    auto itr = std::find(m_hashes.begin(), m_hashes.end(), hash);
    if (itr != m_hashes.cend()) {
        return m_mutators[std::distance(m_hashes.begin(), itr)];
    }

    for (Mutator* mut : m_mutators) {
        for (const auto& alias : mut->m_aliases) {
            if (hash == alias) {
#ifndef _NDEBUG
                std::printf("activating %s\n", mut->m_name.c_str());
#endif
                return mut;
            }
        }
    }
    return nullptr;
}

void MutatorRegistry::activate_mod(Mutator* m) {
    if (devil4_sdk::is_not_in_gameplay()) { return; }
    if (!m->m_enabled) { return; }


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
    auto nameLower = utility::to_lower(name);
    auto hash = std::hash<std::string>{}(nameLower);

    activate_mod(hash);
}

void MutatorRegistry::update(const fmilliseconds dt) {
    for (auto& mutator : m_mutators) {
        if (mutator->m_timer) {
            mutator->m_timer->tick(dt);
        }
    }
}

void MutatorRegistry::save_config(utility::Config& cfg) {
    char m_buffer[512]{0};
    char m_name[512]{0};
    for (size_t i = 0; i < m_hashes.size(); ++i) {
        sprintf_s(m_buffer, sizeof(m_buffer), "%d,%d", m_mutators[i]->m_enabled, m_weights[i]);
        sprintf_s(m_name, sizeof(m_name), "mut%d", i);
        cfg.set(m_name, m_buffer);
    }
}

void MutatorRegistry::load_config(const utility::Config& cfg) {
    char m_name[512]{0};
    char m_val[8]{ 0 };
    char* token1 = NULL;
    char* next_token1 = NULL;
    // TODO(): string stream noncense or find() and substr()?
    for (size_t i = 0; i < m_hashes.size(); ++i) {
        sprintf_s(m_name, sizeof(m_name), "mut%d", i);
        sprintf_s(m_val, sizeof(m_val), "1,%d", m_mutators[i]->m_weight);
        auto string = cfg.get(m_name).value_or(m_val);
        std::vector<std::string> bufs;
        token1 = strtok_s((char*)string.c_str(), ",", &next_token1);
        while ((token1 != NULL))
        {
            // Get next token:
            if (token1 != NULL)
            {
                bufs.push_back(token1);
                token1 = strtok_s(NULL, ",", &next_token1);
            }
        }
        m_weights[i]             = atoi(bufs[1].c_str());
        m_mutators[i]->m_enabled = bufs[0] == "1" ? true : false;
    }
}


Mutator& Mutator::description(const std::string&& description) {
    m_description = description;
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
    auto aliasLower = utility::to_lower(alias);
    m_aliases.emplace_back(std::hash<std::string>{}(aliasLower));
    return *this;
}

Mutator& Mutator::weight(int32_t weight)
{
    m_weight = weight;
    return *this;
}

Mutator& Mutator::special_arg(bool* arg) {
    m_extra_arg = arg;
    return *this;
}

void mutator_draw_mod_table(MutatorRegistry& registry) {
    if (ImGui::CollapsingHeader(_("Gameplay mutators configuration"))) {
        if (ImGui::BeginTable("table1", 3))
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(_("Name"));

            ImGui::TableSetColumnIndex(1);
            ImGui::Text(_("Enabled?"));
            ImGui::SameLine();
            gui::help_marker(_("Toggle commands for chaos mode"));

            ImGui::TableSetColumnIndex(2);
            ImGui::Text(_("Weight"));
            ImGui::SameLine();
            gui::help_marker(_("How often mod will randomly appear during voting"));
            
            size_t hash_index{ 0 };
            for (const auto& mod : registry.m_mutators) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", mod->m_name.c_str());

                char modhash[64];
                sprintf_s(modhash, sizeof(modhash), "##e%d", registry.m_hashes[hash_index]);
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Checkbox(modhash, &mod->m_enabled)) {
                    if (!mod->m_enabled) {
                        registry.m_weights[hash_index] = 0;
                    }
                    else {
                        registry.m_weights[hash_index] = registry.m_mutators[hash_index]->m_weight;
                    }
                }

                sprintf_s(modhash, sizeof(modhash), "##w%d", registry.m_hashes[hash_index]);
                ImGui::TableSetColumnIndex(2);
                ImGui::DragInt(modhash, &registry.m_weights[hash_index], 1, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);

                hash_index += 1;
            }
            ImGui::EndTable();
        }
    }
}

float mutator_is_mod_active(Mutator* mutator) {
    auto timer = mutator->m_timer;
    if (timer) {
        return timer->m_duration - timer->m_time.count();
    }
    return -1.0f;
}

