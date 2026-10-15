#pragma once

// Single seam for all logging in this project. fmtlog 2.3.0 emits C4702
// (unreachable code) inside its own headers under /W4, which we can neither
// fix nor silence via include paths on this generator - so it is scoped out
// here, around the third-party include only. Our own code stays /W4 /WX.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

#include <fmtlog/fmtlog.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
