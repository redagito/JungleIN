#include <iostream>

#include <flextGL.h>
#include <GLFW/glfw3.h>

#include "helpers/utils.h"

int main(int argc, char *argv[]) {
  if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW." << std::endl;
    return 0;
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, FLEXT_MAJOR_VERSION);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, FLEXT_MINOR_VERSION);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window from parameters
  std::cout << "Creating GLFW window for OpenGL version " << FLEXT_MAJOR_VERSION
            << ", " << FLEXT_MINOR_VERSION << std::endl;
  auto window = glfwCreateWindow(1024, 768, "JungleIN", NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window." << std::endl;
    glfwTerminate();
    return 0;
  }

  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);

  // Load OpenGL extensions
  if (flextInit() != GL_TRUE) {
    glfwTerminate();
    std::cout << "Failed to initialize flextGL." << std::endl;
    return 0;
  }

  glfwShowWindow(window);
  Utils::window = window;

  while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();
    glfwSwapBuffers(window);
  }
  return 0;
}
