#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

// Pure helper owned by Lib/source/render/PostComposer.cpp (external
// linkage, declared here so PostComposer.h stays untouched).
// Maps the blur pass target to its resolution scale factor.
float PostComposerBlurFactor(bool useHalf, int halfBuffersRatio);

// Suite covers: the ratio mapping itself (half -> actual member ratio,
// full -> 1). What needs an eye-check: the on-screen blur look after
// the fix (no visual tuning was done, just the correct ratio).
TEST_CASE("BatchA blur factor uses the real half-buffer ratio",
    "[batch-a][post][blur]")
{
  CHECK_THAT(PostComposerBlurFactor(false, 3), WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(PostComposerBlurFactor(false, 4), WithinAbs(1.0f, 1e-6f));
  CHECK_THAT(PostComposerBlurFactor(true, 3), WithinAbs(3.0f, 1e-6f));
  CHECK_THAT(PostComposerBlurFactor(true, 4), WithinAbs(4.0f, 1e-6f));
}
