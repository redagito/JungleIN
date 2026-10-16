#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "helpers/Utils.h"

// Fail fast: a missing file must throw with context instead of
// returning NULL and failing later with a generic message.
TEST_CASE("Utils::getFileContent fails fast", "[helpers][utils]")
{
  CHECK_THROWS_AS(Utils::getFileContent("does-not-exist-junglein.txt"),
                  std::runtime_error);
}

TEST_CASE("Utils::calculFPS reports a positive rate", "[helpers][utils]")
{
  double fps = 0.0;
  for (int i = 0; i < 10000 && fps <= 0.0; ++i)
    fps = Utils::calculFPS();
  CHECK(fps > 0.0);
}
