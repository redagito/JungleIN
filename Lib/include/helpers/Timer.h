#pragma once

#include <chrono>

class Timer {
public:
  void start();
  double restart();

private:
  std::chrono::system_clock::time_point m_start;
};
