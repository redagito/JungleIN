#include "render/LoadingScreen.h"

#include <stdexcept>
#include <string>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "core/Image.h"
#include "helpers/Logger.h"

namespace {
const char* kVertexSource =
    "#version 400\n"
    "layout (location = 0) in vec2 position;\n"
    "layout (location = 1) in vec2 texcoord;\n"
    "out vec2 fTexcoord;\n"
    "void main() {\n"
    "  fTexcoord = texcoord;\n"
    "  gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";
const char* kFragmentSource =
    "#version 400\n"
    "in vec2 fTexcoord;\n"
    "uniform sampler2D texDiffuse;\n"
    "layout (location = 0) out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = texture(texDiffuse, fTexcoord);\n"
    "}\n";
} // namespace

LoadingScreen::LoadingScreen(GLFWwindow* window) : window(window) {
  if (window == nullptr)
    throw std::runtime_error("LoadingScreen needs a window");
}

LoadingScreen::~LoadingScreen() {
  if (ebo != 0)
    glDeleteBuffers(1, &ebo);
  if (vbo != 0)
    glDeleteBuffers(1, &vbo);
  if (vao != 0)
    glDeleteVertexArrays(1, &vao);
  if (texture != 0)
    glDeleteTextures(1, &texture);
  if (program != 0)
    glDeleteProgram(program);
}

void LoadingScreen::checkShader(unsigned int shader, const char* what) {
  int status = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    int logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    std::string log(logSize > 0 ? static_cast<size_t>(logSize) : 1, '\0');
    glGetShaderInfoLog(shader, logSize, nullptr, log.data());
    loge("LoadingScreen {} failed: {}", what, log.c_str());
    throw std::runtime_error("LoadingScreen shader setup failed");
  }
}

void LoadingScreen::checkProgram(unsigned int program) {
  int status = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    int logSize = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    std::string log(logSize > 0 ? static_cast<size_t>(logSize) : 1, '\0');
    glGetProgramInfoLog(program, logSize, nullptr, log.data());
    loge("LoadingScreen link failed: {}", log.c_str());
    throw std::runtime_error("LoadingScreen shader setup failed");
  }
}

void LoadingScreen::ensureResources(const std::string& imagePath) {
  if (program != 0)
    return;

  const unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &kVertexSource, nullptr);
  glCompileShader(vertexShader);
  checkShader(vertexShader, "vertex");

  const unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &kFragmentSource, nullptr);
  glCompileShader(fragmentShader);
  checkShader(fragmentShader, "fragment");

  program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  checkProgram(program);

  const Image image(imagePath);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getWidth(),
               image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               image.getData());

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
}

void LoadingScreen::show(const std::string& imagePath) {
  ensureResources(imagePath);

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0)
    throw std::runtime_error("LoadingScreen has no framebuffer to draw to");

  int imageWidth = 0;
  int imageHeight = 0;
  glBindTexture(GL_TEXTURE_2D, texture);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &imageWidth);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &imageHeight);
  if (imageWidth <= 0 || imageHeight <= 0)
    throw std::runtime_error("LoadingScreen texture has no size");

  // Fit inside the frame, keep aspect, never stretch.
  const float imageAspect =
      static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
  const float frameAspect =
      static_cast<float>(width) / static_cast<float>(height);
  float halfW = 1.0f;
  float halfH = 1.0f;
  if (imageAspect > frameAspect)
    halfH = frameAspect / imageAspect;
  else
    halfW = imageAspect / frameAspect;
  const float vertices[] = {-halfW, -halfH, 0.0f, 0.0f, halfW, -halfH,
                            1.0f,   0.0f,   halfW, halfH, 1.0f, 1.0f,
                            -halfW, halfH,  0.0f, 1.0f};
  const unsigned int indices[] = {0, 1, 2, 0, 2, 3};

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(2 * sizeof(float)));

  glUseProgram(program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindVertexArray(0);
  glUseProgram(0);

  glfwSwapBuffers(window);
}
