#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "helpers/Profiler.h"

namespace {

std::string ReadFile(const std::filesystem::path& p) {
  std::ifstream in(p);
  REQUIRE(in.good());
  std::ostringstream body;
  body << in.rdbuf();
  return body.str();
}

// Parses the trailing "summary,..." line into key=value pairs.
std::string SummaryValue(const std::string& body, const std::string& key) {
  const size_t line = body.rfind("summary,");
  REQUIRE(line != std::string::npos);
  const std::string token = key + "=";
  const size_t pos = body.find(token, line);
  REQUIRE(pos != std::string::npos);
  const size_t end = body.find_first_of(",\n", pos + token.size());
  return body.substr(pos + token.size(), end - pos - token.size());
}

std::filesystem::path ScratchPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

} // namespace

// Synthetic frame times 1..100 ms: mean 50.5, nearest-rank percentiles
// p50=50, p95=95, p99=99, max=100. Pure CPU, no GL.
TEST_CASE("Profiler frame stats math", "[util][profiler]") {
  Profiler::clearFrames();
  for (int i = 1; i <= 100; ++i)
    Profiler::recordFrame(static_cast<double>(i));
  CHECK(Profiler::frameCount() == 100);

  const auto path = ScratchPath("junglein_profiler_math.csv");
  REQUIRE(Profiler::dumpFrameStats(path.string()));
  const std::string body = ReadFile(path);

  // Summary only: no per-sample column.
  CHECK(body.find("sample_ms") == std::string::npos);
  CHECK(body.rfind("summary,", 0) == 0);
  CHECK(SummaryValue(body, "count") == "100");
  CHECK(std::stod(SummaryValue(body, "mean")) ==
    Catch::Approx(50.5).margin(1e-6));
  CHECK(std::stod(SummaryValue(body, "p50")) ==
    Catch::Approx(50.0).margin(1e-6));
  CHECK(std::stod(SummaryValue(body, "p95")) ==
    Catch::Approx(95.0).margin(1e-6));
  CHECK(std::stod(SummaryValue(body, "p99")) ==
    Catch::Approx(99.0).margin(1e-6));
  CHECK(std::stod(SummaryValue(body, "max")) ==
    Catch::Approx(100.0).margin(1e-6));
  Profiler::clearFrames();
  std::filesystem::remove(path);
}

// The ring is bounded: long sessions drop the oldest samples instead of
// growing without limit.
TEST_CASE("Profiler ring cap drops oldest samples", "[util][profiler]") {
  Profiler::clearFrames();
  const std::size_t over = Profiler::kMaxFrameSamples + 100;
  for (std::size_t i = 0; i < over; ++i)
    Profiler::recordFrame(1.0);
  CHECK(Profiler::frameCount() == Profiler::kMaxFrameSamples);

  const auto path = ScratchPath("junglein_profiler_ring.csv");
  REQUIRE(Profiler::dumpFrameStats(path.string()));
  const std::string body = ReadFile(path);
  CHECK(SummaryValue(body, "count") ==
    std::to_string(Profiler::kMaxFrameSamples));
  Profiler::clearFrames();
  std::filesystem::remove(path);
}

// Dump format: one summary line carrying every required aggregate
// token, and nothing else (no per-sample data).
TEST_CASE("Profiler dump file format", "[util][profiler]") {
  Profiler::clearFrames();
  Profiler::recordFrame(16.5);
  Profiler::recordFrame(17.5);

  const auto path = ScratchPath("junglein_profiler_format.csv");
  REQUIRE(Profiler::dumpFrameStats(path.string()));
  const std::string body = ReadFile(path);

  CHECK(body.rfind("summary,", 0) == 0);
  CHECK(body.find('\n') == body.size() - 1);
  for (const std::string& token :
       {"count", "mean", "p50", "p95", "p99", "max"})
    CHECK(body.find(token) != std::string::npos);
  CHECK(SummaryValue(body, "count") == "2");
  CHECK(std::stod(SummaryValue(body, "mean")) ==
    Catch::Approx(17.0).margin(1e-6));
  Profiler::clearFrames();
  std::filesystem::remove(path);
}
