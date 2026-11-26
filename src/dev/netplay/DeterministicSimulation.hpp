#pragma once

#include "../mod.hpp"

class DeterministicSimulation : public Mod {
public:
    DeterministicSimulation() = default;
    std::string get_mod_name() override { return "DeterministicSimulation"; };

    std::optional<std::string> on_initialize() override;

private:
    std::unique_ptr<FunctionHook> g_worker_thread_init_hook, sse_init_hook, sMain_job_proc_hook;
    std::unique_ptr<Patch> fpu_global_init;
};
