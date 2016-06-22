#include "utils.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>

GLFWwindow *Utils::window = nullptr;
bool Utils::USE_ANISO = false;
unsigned int Utils::MAX_ANISO = 0x0000;
int Utils::QUALITY = 1;

double Utils::TARGET_FPS = 32.0;

char *Utils::getFileContent(std::string path) {
  FILE *fp;
  char *content = NULL;
  long length;

  fp = fopen(path.c_str(), "rb");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    content = new char[length + 1];
    fread(content, sizeof(char), length, fp);
    fclose(fp);
    content[length] = '\0';
  }

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

void Timer::start() { m_start = std::chrono::system_clock::now(); }

double Timer::restart() {
  auto now = std::chrono::system_clock::now();
  auto delta =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start);
  m_start = now;
  return delta.count() / 1000.0;
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
