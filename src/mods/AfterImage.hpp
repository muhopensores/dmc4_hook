#pragma once
#ifndef AFTERIMAGE_H
#define AFTERIMAGE_H
#include "../mod.hpp"

class AfterImgCtlVtable {
public:
    std::vector<uintptr_t> my_vtable;
    size_t size;
    AfterImgCtlVtable() = default;
    AfterImgCtlVtable(void* vtable, size_t size);
};

class AfterImgShlVtable {
public:
    std::vector<uintptr_t> my_vtable;
    size_t size;
    AfterImgShlVtable() = default;
    AfterImgShlVtable(void* vtable, size_t size);
};

class AfterImage : public Mod {
public:
    AfterImage() = default;

    static void __stdcall SpawnAfterImgCtl();
    static void __stdcall SpawnAfterImg();
    std::string get_mod_name() override { return "AfterImage"; };
    std::optional<std::string> on_initialize() override;
    // void on_config_load(const utility::Config& cfg) override;
    // void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;
};
#endif