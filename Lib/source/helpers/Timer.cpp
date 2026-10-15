#include "helpers/Timer.h"

#include <chrono>

void Timer::start() { m_start = std::chrono::system_clock::now(); }

double Timer::restart() {
	auto now = std::chrono::system_clock::now();
	auto delta =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start);
	m_start = now;
	return delta.count() / 1000.0;
}
