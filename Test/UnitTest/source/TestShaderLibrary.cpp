#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "render/ShaderLibrary.h"

// Fail fast: requesting an unknown shader must throw so the global
// handler in main can log it, instead of killing the process with exit().
TEST_CASE("ShaderLibrary::getShader fails fast", "[render][shaderlibrary]")
{
  CHECK_THROWS_AS(ShaderLibrary::getShader("does-not-exist-junglein"),
                  std::runtime_error);
}
