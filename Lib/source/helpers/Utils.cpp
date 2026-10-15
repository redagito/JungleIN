#include "helpers/Utils.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include "helpers/Logger.h"

GLFWwindow *Utils::window = nullptr;
bool Utils::USE_ANISO = false;
unsigned int Utils::MAX_ANISO = 0x0000;
int Utils::QUALITY = 1;

double Utils::TARGET_FPS = 32.0;

char *Utils::getFileContent(std::string path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    throw std::runtime_error("Failed to read file: " + path);

  const auto end = file.tellg();
  if (end == std::ifstream::pos_type(-1) || !file.seekg(0))
    throw std::runtime_error("Failed to size file: " + path);

  const size_t length = static_cast<size_t>(end);
  char *content = new char[length + 1];
  if (length > 0) {
    file.read(content, static_cast<std::streamsize>(length));
    if (!file) {
      delete[] content;
      throw std::runtime_error("Failed to read file content: " + path);
    }
  }
  content[length] = '\0';

  return content;
}

double Utils::calculFPS() {
  static double current_time = 0;
  static double last_time = 0;
  static double average = 0;
  static int count = 0;
  static double av = 0;
  last_time = current_time;

  // QTime t;
  current_time = 0; // t.currentTime().msec();

  if (current_time > last_time) {
    average += 1000 / (current_time - last_time);
    count++;
  }

  if (count == 30) {

    // Utils::MAIN->getFPSDisplay()->display(average / count);
    av = average / count;
    count = 0;
    average = 0;
  }
  return av;
}

double Utils::random(double min, double max) {
  return (rand() / static_cast<double>(RAND_MAX)) * (max - min) + min;
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
