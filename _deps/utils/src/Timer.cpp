#include <utils/Timer.h>

namespace utils {
	Timer::Timer() : m_stopped(true), m_pausedAt(0.0f) {
	}

	Timer::~Timer() {
	}

	void Timer::start() {
		if (!m_stopped) return;
		m_startPoint = std::chrono::high_resolution_clock::now();
		m_stopped = false;
	}

	void Timer::pause() {
		if (m_stopped) return;
		m_pausedAt = elapsed();
		m_stopped = true;
	}

	void Timer::reset() {
		m_stopped = true;
		m_pausedAt = 0.0f;
	}

	bool Timer::stopped() const {
		return m_stopped;
	}

	f32 Timer::elapsed() const {
		if (m_stopped) return m_pausedAt;
		return m_pausedAt + std::chrono::duration<f32>(std::chrono::high_resolution_clock::now() - m_startPoint).count();
	}

	Timer::operator f32() const {
		if (m_stopped) return m_pausedAt;
		return m_pausedAt + std::chrono::duration<f32>(std::chrono::high_resolution_clock::now() - m_startPoint).count();
	}
};