#pragma once

#include <cstddef>
#include <string>

class Profiler {
public:
  // One-shot milestone profiler: logs milliseconds since the first mark.
  // Startup/first-frame only - never call per frame.
  static void mark(const char* stage);

  // Per-frame profiling: bounded ring of the last kMaxFrameSamples frame
  // times in milliseconds (drop-oldest, so long sessions stay bounded).
  // recordFrame never logs: zero per-frame logging is a hard requirement.
  // dumpFrameStats writes one summary line with
  // count/mean/p50/p95/p99/max (no per-sample data: a long session must
  // not produce a megabyte dump) and returns true on success.
  static constexpr std::size_t kMaxFrameSamples = 60000;
  static void recordFrame(double ms);
  static bool dumpFrameStats(const std::string& path);
  static std::size_t frameCount();
  static void clearFrames();
};
