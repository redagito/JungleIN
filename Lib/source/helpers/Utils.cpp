#include "helpers/Utils.h"

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <random>
#include <stdexcept>
#include "helpers/Logger.h"

std::atomic<GLFWwindow*> Utils::window{nullptr};
std::atomic<bool> Utils::USE_ANISO{false};
std::atomic<float> Utils::MAX_ANISO{1.0f};
std::atomic<int> Utils::QUALITY{1};

std::atomic<double> Utils::TARGET_FPS{32.0};

namespace {
std::mutex& FpsMutex() {
  static std::mutex mutex;
  return mutex;
}
} // namespace

double Utils::movementDelta(double elapsedSeconds) {
  return elapsedSeconds / TARGET_FPS.load();
}

std::string Utils::getFileContent(std::string path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    throw std::runtime_error("Failed to read file: " + path);

  const auto end = file.tellg();
  if (end == std::ifstream::pos_type(-1) || !file.seekg(0))
    throw std::runtime_error("Failed to size file: " + path);

  const size_t length = static_cast<size_t>(end);
  std::string content(length, '\0');
  if (length > 0) {
    file.read(content.data(), static_cast<std::streamsize>(length));
    if (!file)
      throw std::runtime_error("Failed to read file content: " + path);
  }

  return content;
}

double Utils::calculFPS() {
  using namespace std::chrono;
  std::lock_guard<std::mutex> lock(FpsMutex());
  static steady_clock::time_point lastCall = steady_clock::now();
  static double rateTotal = 0.0;
  static int sampleCount = 0;
  static double reportedFps = 0.0;

  const double seconds =
      duration_cast<duration<double>>(steady_clock::now() - lastCall).count();
  lastCall = steady_clock::now();
  if (seconds > 0.0) {
    rateTotal += 1.0 / seconds;
    ++sampleCount;
  }

  if (sampleCount >= 30) {
    reportedFps = rateTotal / sampleCount;
    rateTotal = 0.0;
    sampleCount = 0;
  }
  return reportedFps;
}

double Utils::random(double min, double max) {
  thread_local std::mt19937 engine{std::random_device{}()};
  std::uniform_real_distribution<double> dist(min, max);
  return dist(engine);
}

std::vector<std::string> &operator<<(std::vector<std::string> &stringList,
                                     const std::string &str) {
  stringList.push_back(str);
  return stringList;
}

std::vector<std::string> operator<<(std::vector<std::string> stringList,
                                    const char *str) {
  return stringList << std::string(str);
}
