#pragma once

using fmilliseconds = std::chrono::duration<float, std::milli>;
using fseconds = std::chrono::duration<float>;
using fminutes = std::chrono::duration<float, std::ratio<60>>;
using fhours = std::chrono::duration<float, std::ratio<3600>>;
using fdays = std::chrono::duration<float, std::ratio<86400>>;
using fweeks = std::chrono::duration<float, std::ratio<604800>>;
using fmonths = std::chrono::duration<float, std::ratio<2629746>>;
using fyears = std::chrono::duration<float, std::ratio<31556952>>;