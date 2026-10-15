#include "helpers/Profiler.h"

#include <chrono>

#include "helpers/Logger.h"

void Profiler::mark(const char* stage) {
	using namespace std::chrono;
	static const auto start = steady_clock::now();
	static auto previous = start;
	const auto now = steady_clock::now();
	const auto total =
		duration_cast<milliseconds>(now - start).count();
	const auto delta =
		duration_cast<milliseconds>(now - previous).count();
	previous = now;
	logi("Startup {}ms (+{}ms): {}", total, delta, stage);
}
