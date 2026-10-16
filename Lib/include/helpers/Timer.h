#pragma once

#include <chrono>

class Timer {
public:
  void start();
  double restart();

private:
  // Monotonic: wall-clock jumps (NTP, suspend) must never produce a
  // negative or truncated frame delta.
  std::chrono::steady_clock::time_point m_start;
};
