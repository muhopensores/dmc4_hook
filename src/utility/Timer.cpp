#include "Timer.hpp"

namespace utility {

	void Timer::start() {
		m_active = true;
		m_time = fseconds{ 0 };
	}

	void Timer::stop() {
		m_active = false;
		m_time = fseconds{ 0 };
	}

	void Timer::tick(const fmilliseconds& dt) {
		if (!m_active) { return; }
		if (m_time.count() >= m_duration) {
			if (m_callback) {
				m_callback();
			}
			m_active = false;
		}
		m_time += dt;
	}
};