#pragma once
#include "../mod.hpp"

class MultipleGameInstances : public Mod {
public:
    MultipleGameInstances() = default;
    std::string get_mod_name() override { return "MultipleGameInstances"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    std::optional<std::string> on_initialize() override;
};
