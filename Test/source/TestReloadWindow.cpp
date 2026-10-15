#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "ui/ReloadWindow.h"

// Density/spacing feed allocation loops: a negative value would wrap the
// unsigned members to billions and OOM. Fail fast instead.
TEST_CASE("ReloadWindow rejects negative parameters", "[ui][reload]")
{
  SECTION("negative density throws")
  {
    ReloadWindow window;
    CHECK_THROWS_AS(window.setDensity(-5), std::runtime_error);
  }

  SECTION("negative spacing throws")
  {
    ReloadWindow window;
    CHECK_THROWS_AS(window.setSpacing(-10), std::runtime_error);
  }

  SECTION("valid parameters stick")
  {
    ReloadWindow window;
    window.setDensity(100);
    window.setSpacing(50);
    CHECK(window.getDensity() == 100);
    CHECK(window.getSpacing() == 50);
  }
}
