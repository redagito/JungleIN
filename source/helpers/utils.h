#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

#include <GLFW/glfw3.h>

/*!
 * Utils
 * Fonctions diverses et vari�es (mais utiles)
 * @static
 */
class Utils {
public:
  static char *getFileContent(std::string path);

  static GLFWwindow *window;
  static bool USE_ANISO;
  static unsigned int MAX_ANISO;
  static int QUALITY;

  static double TARGET_FPS;

  static double calculFPS();

  static double random(double min, double max);
};

class Timer {
public:
  void start();
  double restart();

private:
  std::chrono::system_clock::time_point m_start;
};

template <typename T> size_t indexOf(const std::vector<T> &values, T value) {
  for (size_t i = 0; i < values.size(); ++i) {
    if (values.at(i) == value) {
      return i;
    }
  }
  exit(-1);
  return 0;
}

template <typename K, typename V>
V value(const std::unordered_map<K, V> &data, K key, V def) {
  if (data.count(key) == 0) {
    return def;
  }
  return data.at(key);
}

std::vector<std::string> &operator<<(std::vector<std::string> &stringList,
                                     const std::string &str);
std::vector<std::string> operator<<(std::vector<std::string> stringList,
                                    const char *str);

#endif // UTILS_H
