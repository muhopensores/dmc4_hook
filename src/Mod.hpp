#pragma once

#define naked static __declspec(naked)
#define _(string) utility::text_lookup(string)
#define __(str) str

#include "ModPCH.hpp"
