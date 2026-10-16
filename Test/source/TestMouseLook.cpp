#include <catch2/catch_test_macros.hpp>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "ui/Window.h"

// Releasing the left button ends hold-to-look, but must not strand an
// F1-captured view: under capture (cursor disabled) look stays on until
// F2 or Escape. Pure policy, no GL context needed.
TEST_CASE("Mouse release keeps captured look", "[ui][mouse]") {
  CHECK(Window::keepLookOnButtonRelease(GLFW_CURSOR_DISABLED));
  CHECK_FALSE(Window::keepLookOnButtonRelease(GLFW_CURSOR_NORMAL));
  CHECK_FALSE(Window::keepLookOnButtonRelease(GLFW_CURSOR_HIDDEN));
}
