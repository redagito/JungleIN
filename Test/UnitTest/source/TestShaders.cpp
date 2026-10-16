#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// All project shaders declare "#version 400". Under a core profile the
// compatibility sampler function texture2D() does not exist and shader
// compilation fails, so every shader must use texture() instead.
TEST_CASE("Shaders use core-profile sampler functions", "[render][shaders]")
{
  const std::filesystem::path shaderDir =
      std::filesystem::path(JUNGLEIN_DATA_DIR) / "shaders";
  REQUIRE(std::filesystem::is_directory(shaderDir));

  size_t checked = 0;
  for (const auto &entry : std::filesystem::directory_iterator(shaderDir)) {
    if (entry.path().extension() != ".vs" &&
        entry.path().extension() != ".fs")
      continue;
    std::ifstream in(entry.path());
    REQUIRE(in.good());
    std::ostringstream body;
    body << in.rdbuf();
    INFO("shader file: " << entry.path().filename().string());
    CHECK(body.str().find("texture2D(") == std::string::npos);
    ++checked;
  }
  CHECK(checked > 0);
}
