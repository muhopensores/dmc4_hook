#pragma once

#include "../mod.hpp"

class LiveMoveTableSwaps : public Mod {
public:
    LiveMoveTableSwaps() = default;

    static bool instant_honeycomb_enabled;
    static uintptr_t live_move_table_swaps_jmp_ret;

    std::string get_mod_name() override { return "LiveMoveTableSwaps"; };
    std::vector<std::string> get_search_terms() override {
        return {"instant honeycomb", "back forward honeycomb"
        };
    }
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> live_move_table_swaps_hook;
};
