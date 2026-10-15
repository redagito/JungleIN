#include "helpers/Profiler.h"

#include <chrono>

#include "helpers/Logger.h"

void Profiler::mark(const char* stage) {
	using namespace std::chrono;
	static const auto start = steady_clock::now();
	const auto ms =
		duration_cast<milliseconds>(steady_clock::now() - start).count();
	logi("Startup {}ms: {}", ms, stage);
}
