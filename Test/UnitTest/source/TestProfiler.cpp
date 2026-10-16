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

// Sections: per-pass timings accumulate in bounded rings and report
// sorted by total time descending, so the biggest loser is first.
// Pure CPU, no GL, no logging.
TEST_CASE("Profiler sections sort by total time", "[util][profiler]") {
  Profiler::clearSections();
  Profiler::recordSection("shadow", 3.0);
  Profiler::recordSection("shadow", 5.0);
  Profiler::recordSection("meshes", 10.0);

  const auto stats = Profiler::sectionStats();
  REQUIRE(stats.size() == 2);
  // meshes total 10.0 beats shadow total 8.0.
  CHECK(stats[0].name == "meshes");
  CHECK(stats[0].count == 1);
  CHECK(stats[0].total == Catch::Approx(10.0).margin(1e-9));
  CHECK(stats[0].mean == Catch::Approx(10.0).margin(1e-9));
  CHECK(stats[0].max == Catch::Approx(10.0).margin(1e-9));
  CHECK(stats[1].name == "shadow");
  CHECK(stats[1].count == 2);
  CHECK(stats[1].total == Catch::Approx(8.0).margin(1e-9));
  CHECK(stats[1].mean == Catch::Approx(4.0).margin(1e-9));
  CHECK(stats[1].max == Catch::Approx(5.0).margin(1e-9));
  Profiler::clearSections();
  CHECK(Profiler::sectionStats().empty());
}

// Running aggregates stay in sync with the ring across eviction: the
// reported means must reflect the last kMaxSectionSamples only, not the
// session lifetime (observer cost stays O(1) per frame either way).
TEST_CASE("Profiler section aggregates follow eviction", "[util][profiler]") {
  Profiler::clearSections();
  for (std::size_t i = 0; i < Profiler::kMaxSectionSamples; ++i)
    Profiler::recordSection("shadow", 1.0);
  for (int i = 0; i < 100; ++i)
    Profiler::recordSection("shadow", 3.0);
  const auto stats = Profiler::sectionStats();
  REQUIRE(stats.size() == 1);
  CHECK(stats[0].count == Profiler::kMaxSectionSamples);
  CHECK(stats[0].total == Catch::Approx(500.0 * 1.0 + 100.0 * 3.0).margin(1e-6));
  CHECK(stats[0].mean == Catch::Approx(800.0 / 600.0).margin(1e-9));
  CHECK(stats[0].max == Catch::Approx(3.0).margin(1e-9));
  Profiler::clearSections();
}

TEST_CASE("Profiler section max resets after eviction", "[util][profiler]") {
  Profiler::clearSections();
  Profiler::recordSection("shadow", 50.0);
  for (std::size_t i = 0; i < Profiler::kMaxSectionSamples; ++i)
    Profiler::recordSection("shadow", 1.0);
  const auto stats = Profiler::sectionStats();
  REQUIRE(stats.size() == 1);
  CHECK(stats[0].count == Profiler::kMaxSectionSamples);
  CHECK(stats[0].max == Catch::Approx(1.0).margin(1e-9));
  CHECK(stats[0].total == Catch::Approx(600.0).margin(1e-6));
  Profiler::clearSections();
}

// Section rings are bounded: old samples drop out, so per-frame stats
// cost stays flat no matter how long the session runs.
TEST_CASE("Profiler section rings drop oldest samples", "[util][profiler]") {
  Profiler::clearSections();
  const std::size_t over = Profiler::kMaxSectionSamples + 100;
  for (std::size_t i = 0; i < over; ++i)
    Profiler::recordSection("shadow", 2.0);
  const auto stats = Profiler::sectionStats();
  REQUIRE(stats.size() == 1);
  CHECK(stats[0].count == Profiler::kMaxSectionSamples);
  CHECK(stats[0].total ==
    Catch::Approx(2.0 * Profiler::kMaxSectionSamples).margin(1e-6));
  Profiler::clearSections();
}

TEST_CASE("Profiler Scope records elapsed time", "[util][profiler]") {
  Profiler::clearSections();
  {
    const Profiler::Scope scope("update");
    CHECK(Profiler::sectionStats().empty());
  }
  const auto stats = Profiler::sectionStats();
  REQUIRE(stats.size() == 1);
  CHECK(stats[0].name == "update");
  CHECK(stats[0].count == 1);
  CHECK(stats[0].total >= 0.0);
  Profiler::clearSections();
}

TEST_CASE("Profiler section dump file format", "[util][profiler]") {
  Profiler::clearSections();
  Profiler::recordSection("shadow", 3.0);
  Profiler::recordSection("meshes", 10.0);

  const auto path = ScratchPath("junglein_profiler_sections.csv");
  REQUIRE(Profiler::dumpSectionStats(path.string()));
  const std::string body = ReadFile(path);

  // One section= line per section, biggest total first, trailing newline.
  CHECK(body.rfind("section,", 0) == 0);
  REQUIRE(!body.empty());
  CHECK(body.back() == '\n');
  CHECK(body.find("name=meshes") != std::string::npos);
  CHECK(body.find("name=shadow") != std::string::npos);
  CHECK(body.find("name=meshes") < body.find("name=shadow"));
  CHECK(body.find("total=") != std::string::npos);
  CHECK(body.find("recentMean=") != std::string::npos);
  Profiler::clearSections();
  std::filesystem::remove(path);
}

// Trend window: recentMean covers the newest kRecentSamples, so a
// climbing section reads above its ring mean and a settling one below.
TEST_CASE("Profiler section recentMean tracks the tail", "[util][profiler]") {
  Profiler::clearSections();
  for (int i = 0; i < 100; ++i)
    Profiler::recordSection("shadow", 1.0);
  for (int i = 0; i < 10; ++i)
    Profiler::recordSection("shadow", 5.0);
  const auto stats = Profiler::sectionStats();
  REQUIRE(stats.size() == 1);
  CHECK(stats[0].count == 110);
  CHECK(stats[0].mean == Catch::Approx(150.0 / 110.0).margin(1e-9));
  // Tail window holds the last 60: fifty 1.0 plus ten 5.0.
  CHECK(stats[0].recentMean == Catch::Approx(100.0 / 60.0).margin(1e-9));
  CHECK(stats[0].recentMean > stats[0].mean);
  Profiler::clearSections();
  CHECK(Profiler::sectionStats().empty());
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
