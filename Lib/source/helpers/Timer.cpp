#include "helpers/Timer.h"

#include <chrono>

void Timer::start() { m_start = std::chrono::steady_clock::now(); }

double Timer::restart() {
	auto now = std::chrono::steady_clock::now();
	// Fractional seconds: truncating to milliseconds quantized every
	// sub-millisecond frame delta to zero.
	double delta =
		std::chrono::duration<double>(now - m_start).count();
	m_start = now;
	return delta;
}
