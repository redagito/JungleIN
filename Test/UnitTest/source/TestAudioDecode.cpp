#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <stdexcept>

#include "audio/AudioClip.h"

using Catch::Matchers::WithinAbs;

// The ambient loop ships with the game data; decoding it must yield
// playable PCM. Missing files fail fast instead of silence.
TEST_CASE("AudioClip decodes MP3 to PCM", "[audio][clip]")
{
  SECTION("ambiant loop decodes to non-empty PCM")
  {
    const std::string file =
        (std::filesystem::path(JUNGLEIN_DATA_DIR) / "resources" / "sounds" /
         "ambiant.mp3")
            .string();
    REQUIRE(std::filesystem::exists(file));

    const AudioClip clip = AudioClip::decodeMP3(file);
    CHECK(!clip.samples.empty());
    CHECK((clip.channels == 1 || clip.channels == 2));
    CHECK(clip.sampleRate > 0);
    CHECK(clip.samples.size() % static_cast<size_t>(clip.channels) == 0);
  }

  SECTION("missing file fails fast")
  {
    CHECK_THROWS_AS(AudioClip::decodeMP3("does-not-exist-junglein.mp3"),
                    std::runtime_error);
  }
}
