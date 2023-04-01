#pragma once
#include <chrono>
#include <functional>
#include <deque>

#include "ChronoAlias.hpp"

namespace utility {
	// TODO(): this is probably horrible by bideogame standards
	// replace std::function with c function pointers or something
	struct Timer {

		Timer(float seconds, std::function<void()> callback)
			: m_duration(seconds), m_callback(callback) {
		}
		Timer(Timer&& other) = delete;

		bool     m_active{ false };
		float    m_duration;
		fseconds m_time { 0 };

		std::function<void()> m_callback;

		void start();
		void stop();

		void tick(const fmilliseconds& dt);
	};
}; // namespace utility