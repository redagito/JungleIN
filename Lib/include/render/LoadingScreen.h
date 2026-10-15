#pragma once

#include <string>

struct GLFWwindow;

// Init-only splash: clears black, draws one image centered without
// stretching, presents. Never used inside the render loop.
class LoadingScreen {
public:
  explicit LoadingScreen(GLFWwindow* window);
  ~LoadingScreen();

  LoadingScreen(const LoadingScreen&) = delete;
  LoadingScreen& operator=(const LoadingScreen&) = delete;

  void show(const std::string& imagePath);

private:
  void ensureResources(const std::string& imagePath);
  static void checkShader(unsigned int shader, const char* what);
  static void checkProgram(unsigned int program);

  GLFWwindow* window = nullptr;
  unsigned int program = 0;
  unsigned int texture = 0;
  unsigned int vao = 0;
  unsigned int vbo = 0;
  unsigned int ebo = 0;
};
