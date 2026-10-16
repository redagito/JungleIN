#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

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

  // Per-section profiling: same no-logging contract as recordFrame, but
  // split by pass name ("shadow", "meshes", "post.bloom", ...) so the
  // frame budget shows where time goes instead of a single total.
  // Each section keeps a bounded ring (drop-oldest); section count is
  // capped too - unknown names past the cap are dropped, never logged.
  struct SectionStats {
    std::string name;
    std::size_t count = 0;
    double mean = 0.0;
    double max = 0.0;
    double total = 0.0;
  };
  static constexpr std::size_t kMaxSectionSamples = 600;
  static constexpr std::size_t kMaxSections = 32;
  static void recordSection(const char* name, double ms);
  // Aggregates over the recorded rings, hottest total first.
  static std::vector<SectionStats> sectionStats();
  static void clearSections();
  // One section= line per section (sectionStats order), returns true
  // on success. Separate file from dumpFrameStats: that format is a
  // pinned single summary line.
  static bool dumpSectionStats(const std::string& path);

  // RAII stopwatch: records steady_clock elapsed milliseconds into the
  // named section on destruction. Never logs, never throws. The name
  // must outlive the scope (string literals at call sites).
  class Scope {
  public:
    explicit Scope(const char* name);
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
  private:
    const char* name_;
    std::chrono::steady_clock::time_point start_;
  };
};
