#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "render/Shader.h"
#include "render/ShaderLibrary.h"

// Fail fast: requesting an unknown shader must throw so the global
// handler in main can log it, instead of killing the process with exit().
TEST_CASE("ShaderLibrary::getShader fails fast", "[render][shaderlibrary]")
{
  CHECK_THROWS_AS(ShaderLibrary::getShader("does-not-exist-junglein"),
                  std::runtime_error);
}

// Hot-loop location resolve: an unregistered uniform must throw the
// same fail-fast error as transmitting it (resolve happens once per
// material bind instead of once per instance). No GL context needed:
// an unloaded Shader carries no locations.
TEST_CASE("Shader requireLocation fails fast", "[render][shader]")
{
  Shader probe("junglein-probe");
  CHECK_THROWS_AS(probe.requireLocation("mvp"), std::runtime_error);
}
