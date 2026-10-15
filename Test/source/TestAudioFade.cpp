#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio/AudioEngine.h"

using Catch::Matchers::WithinAbs;

// The ambient loop fades in linearly over 3 seconds on (re)start,
// then holds full gain while looping.
TEST_CASE("AudioEngine fade gain", "[audio][fade]")
{
  CHECK_THAT(AudioEngine::fadeGain(-1.0), WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(AudioEngine::fadeGain(0.0), WithinAbs(0.0f, 1e-6f));
  CHECK_THAT(AudioEngine::fadeGain(1.5), WithinAbs(0.5f, 1e-6f));
  CHECK_THAT(AudioEngine::fadeGain(3.0), WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(AudioEngine::fadeGain(300.0), WithinAbs(1.0f, 1e-6f));
}
