#include "helpers/Profiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <vector>

#include "helpers/Logger.h"

namespace {

std::deque<double>& FrameSamples() {
  static std::deque<double> samples;
  return samples;
}

// Nearest-rank percentile over ascending sorted data (rank = ceil(q*n)).
// Empty input yields 0.0.
double Percentile(const std::vector<double>& sorted, double q) {
  if (sorted.empty())
    return 0.0;
  const double clamped = std::clamp(q, 0.0, 1.0);
  std::size_t rank =
    static_cast<std::size_t>(std::ceil(clamped * sorted.size()));
  if (rank < 1)
    rank = 1;
  if (rank > sorted.size())
    rank = sorted.size();
  return sorted[rank - 1];
}

double Mean(const std::vector<double>& values) {
  if (values.empty())
    return 0.0;
  double total = 0.0;
  for (double v : values)
    total += v;
  return total / static_cast<double>(values.size());
}

} // namespace

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

void Profiler::recordFrame(double ms) {
  // Hot path: no allocation beyond the bounded deque, no logging.
  std::deque<double>& samples = FrameSamples();
  samples.push_back(ms);
  while (samples.size() > kMaxFrameSamples)
    samples.pop_front();
}

bool Profiler::dumpFrameStats(const std::string& path) {
  const std::vector<double> samples(
    FrameSamples().begin(), FrameSamples().end());
  std::vector<double> sorted = samples;
  std::sort(sorted.begin(), sorted.end());
  double maxMs = 0.0;
  for (double v : samples) {
    if (!std::isfinite(v))
      continue;
    if (v > maxMs)
      maxMs = v;
  }
  std::ofstream out(path, std::ios::out | std::ios::trunc);
  if (!out.good())
    return false;
  // Summary only: per-sample data would turn a long session into a
  // multi-megabyte dump for no diagnostic gain.
  out << std::setprecision(10) << std::fixed;
  out << "summary,count=" << samples.size()
    << ",mean=" << Mean(samples)
    << ",p50=" << Percentile(sorted, 0.50)
    << ",p95=" << Percentile(sorted, 0.95)
    << ",p99=" << Percentile(sorted, 0.99)
    << ",max=" << maxMs << "\n";
  out.flush();
  return static_cast<bool>(out);
}

std::size_t Profiler::frameCount() { return FrameSamples().size(); }

void Profiler::clearFrames() { FrameSamples().clear(); }
