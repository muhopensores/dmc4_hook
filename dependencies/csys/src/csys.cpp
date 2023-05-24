// Copyright(c) 2020-present, Roland Munguia & Tristan Florian Bouchard.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef CSYS_COMPILED_LIB
#error Please define CSYS_COMPILE_LIB to compile this file.
#endif

// We add .inl into .cpp to create a entry point to build everything from.
#include "csys/autocomplete.inl"
#include "csys/system.inl"
#include "csys/history.inl"
#include "csys/item.inl"
#include "csys/script.inl"
