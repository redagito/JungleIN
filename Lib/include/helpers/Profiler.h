#pragma once

class Profiler {
public:
  // One-shot milestone profiler: logs milliseconds since the first mark.
  // Startup/first-frame only - never call per frame.
  static void mark(const char* stage);
};
