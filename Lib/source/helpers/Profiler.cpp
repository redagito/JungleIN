#include "helpers/Profiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "helpers/Logger.h"

namespace {

std::mutex& ProfilerMutex() {
  static std::mutex mutex;
  return mutex;
}

std::deque<double>& FrameSamples() {
  static std::deque<double> samples;
  return samples;
}

// Running aggregates per section: sectionStats must stay O(sections),
// never O(samples). Scanning rings per frame would charge the measured
// overlay scope for observer work that grows while rings fill.
struct SectionRing {
  std::deque<double> samples;
  double total = 0.0;
  double max = 0.0;
  // Bounded tail window with its own running sum: feeds recentMean so
  // sectionStats can report a trend without scanning any ring.
  // Finite samples only (mirrors total/max semantics).
  std::deque<double> recent;
  double recentSum = 0.0;
};

std::unordered_map<std::string, SectionRing>& SectionRings() {
  static std::unordered_map<std::string, SectionRing> rings;
  return rings;
}

double RingMax(const std::deque<double>& samples) {
  double max = 0.0;
  for (double v : samples) {
    if (!std::isfinite(v))
      continue;
    if (v > max)
      max = v;
  }
  return max;
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
  std::lock_guard<std::mutex> lock(ProfilerMutex());
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
  std::lock_guard<std::mutex> lock(ProfilerMutex());
  std::deque<double>& samples = FrameSamples();
  samples.push_back(ms);
  while (samples.size() > kMaxFrameSamples)
    samples.pop_front();
}

bool Profiler::dumpFrameStats(const std::string& path) {
  std::vector<double> samples;
  {
    std::lock_guard<std::mutex> lock(ProfilerMutex());
    samples.assign(FrameSamples().begin(), FrameSamples().end());
  }
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

std::size_t Profiler::frameCount() {
  std::lock_guard<std::mutex> lock(ProfilerMutex());
  return FrameSamples().size();
}

void Profiler::clearFrames() {
  std::lock_guard<std::mutex> lock(ProfilerMutex());
  FrameSamples().clear();
}

void Profiler::recordSection(const char* name, double ms) {
  if (name == nullptr)
    return;
  std::lock_guard<std::mutex> lock(ProfilerMutex());
  auto& rings = SectionRings();
  auto found = rings.find(name);
  if (found == rings.end()) {
    if (rings.size() >= kMaxSections)
      return;
    found = rings.emplace(name, SectionRing()).first;
  }
  SectionRing& ring = found->second;
  if (std::isfinite(ms)) {
    ring.total += ms;
    if (ms > ring.max)
      ring.max = ms;
    ring.recent.push_back(ms);
    ring.recentSum += ms;
    // Recent holds finite samples only, so the evicted value always
    // counts and needs no finiteness check.
    if (ring.recent.size() > kRecentSamples) {
      ring.recentSum -= ring.recent.front();
      ring.recent.pop_front();
    }
  }
  // Non-finite samples still occupy ring slots (count semantics
  // unchanged) but never enter totals, maxima or the trend window.
  ring.samples.push_back(ms);
  while (ring.samples.size() > kMaxSectionSamples) {
    const double evicted = ring.samples.front();
    ring.samples.pop_front();
    // Keep the running aggregates exactly in sync with the ring:
    // otherwise the reported means would drift as sessions run long.
    if (std::isfinite(evicted)) {
      ring.total -= evicted;
      if (evicted >= ring.max)
        ring.max = RingMax(ring.samples);
    }
  }
}

std::vector<Profiler::SectionStats> Profiler::sectionStats() {
  // Zero-copy read of running aggregates under one lock hold: the caller
  // is often the per-frame overlay itself, so any per-sample work here
  // would charge the measured scope for its own observer overhead.
  std::lock_guard<std::mutex> lock(ProfilerMutex());
  const auto& rings = SectionRings();
  std::vector<SectionStats> stats;
  stats.reserve(rings.size());
  for (const auto& entry : rings) {
    const SectionRing& ring = entry.second;
    if (ring.samples.empty())
      continue;
    SectionStats s;
    s.name = entry.first;
    s.count = ring.samples.size();
    s.total = ring.total;
    s.max = ring.max;
    s.mean = s.total / static_cast<double>(s.count);
    s.recentMean = ring.recent.empty()
      ? 0.0
      : ring.recentSum / static_cast<double>(ring.recent.size());
    stats.push_back(std::move(s));
  }
  std::sort(stats.begin(), stats.end(),
    [](const SectionStats& a, const SectionStats& b) {
      return a.total > b.total;
    });
  return stats;
}

void Profiler::clearSections() {
  std::lock_guard<std::mutex> lock(ProfilerMutex());
  SectionRings().clear();
}

bool Profiler::dumpSectionStats(const std::string& path) {
  const std::vector<SectionStats> stats = sectionStats();
  std::ofstream out(path, std::ios::out | std::ios::trunc);
  if (!out.good())
    return false;
  out << std::setprecision(6) << std::fixed;
  for (const SectionStats& s : stats) {
    out << "section,name=" << s.name
      << ",count=" << s.count
      << ",mean=" << s.mean
      << ",max=" << s.max
      << ",total=" << s.total
      << ",recentMean=" << s.recentMean << "\n";
  }
  out.flush();
  return static_cast<bool>(out);
}

Profiler::Scope::Scope(const char* name)
  : name_(name), start_(std::chrono::steady_clock::now()) {
}

Profiler::Scope::~Scope() {
  using namespace std::chrono;
  const double ms =
    duration_cast<duration<double, std::milli>>(steady_clock::now() - start_)
      .count();
  recordSection(name_, ms);
}
