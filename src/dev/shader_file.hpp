#pragma once

// Loose shader sources (Graphics tab > Shader files). PbrMaterials, the shadow receiver (Shadows) and ToneMap each carry
// their HLSL built in and compile it at run time; while nativePC\shader\<mod>\<name>.hlsl exists it is compiled instead,
// so the shaders can be edited without a build. One list for the three, because a file written by an older dmc4_hook
// silently keeps newer built-in code out (Shadows step 3 needs the PBR shader's shadow code, 2026-11-15): the list
// compares each file with the built-in source and says when they differ.
namespace shader_file {

struct Source {
    const char* name;           // shown in the list ("PBR materials")
    const char* path;           // nativePC\shader\...\*.hlsl
    const char* embedded;       // the built-in source
    void (*recompile)();        // compile again (any thread)
    int order;                  // place in the list
};

// Lists a shader; call once, at static init or on_initialize. Returns true (for `const bool listed = add(...)`).
bool add(const Source& source);

// Graphics tab > Shader files: per shader, whether it runs the built-in source or a file (the same or not), and
// Write file / Reload / Use built in. A file that is replaced or put aside is kept as <path>.bak.
void draw();

} // namespace shader_file
