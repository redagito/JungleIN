# Refactor of the JungleIN project

Modernized OpenGL jungle viewer: `App` holds the executable entry point,
`Lib` the engine as a static library, `Test` the Catch2 test suite.

## Requirements

Development is done with
* Conan 2.32.0
* CMake 4.4.3 (minimum CMake 4.4)
* Visual Studio Community 2026 (MSVC 19.51)
* C++23

Dependencies are managed with Conan; see `conanfile.txt` for the
pinned versions.

## Layout

* `App/` - the `JungleINApp` executable, only `source/Main.cpp`
* `Lib/` - the `JungleINLib` static library, `source/` + `include/`
* `Test/` - the `JungleINTests` Catch2 suite, `source/`
* `data/` - runtime assets (shaders, textures, heightmaps, meshes)

The root `CMakeLists.txt` is only an umbrella: toolchain settings plus
`add_subdirectory` for `Lib`, `App` and `Test`.

## Building

Conan install for debug and release build types
```
conan install . --build=missing --settings=build_type=Debug && conan install . --build=missing --settings=build_type=Release
```

CMake project generation
```
cmake --preset conan-default
```

Build in release mode
```
cd build
cmake --build . --config Release
```

Run the tests
```
ctest --preset conan-debug --output-on-failure
```

## Controls

Movement - Arrow Keys / WASD

Fly up / down - E / Q

Look around - hold left mouse button, or F1 (capture) / F2 (release)

Effects - 1 / 2 (FXAA off / on), 3 / 4 (lens flares off / on)

Exposure - 5 / 6 (auto-exposure off / on), 7 / 8 (bias down / up)

Quit - Escape

## Renderer

Linear HDR pipeline: lit shaders work in linear (sRGB inputs decoded
on sample), shadows are 2-cascade PCF from 24-bit depth maps, and the
final display pass applies auto-exposure (log-average luminance with
temporal adaptation), ACES tonemapping and gamma 2.2. Per-frame
exposure diagnostics are logged as `autoexposure ...` lines.

# Original Readme
JungleIN
========

An OpenGL jungle viewer: a small open-world tropical scene you can walk
and fly through, written as a scholar project.

[Web page of the project](http://bkcore.com/blog/3d/opengl-jungle-viewer-source-utbm.html)
[Youtube demo video](http://youtu.be/4M-wfzuCgvs)

Overview: dense procedurally scattered vegetation (trees, plants, rocks)
on heightmapped terrain under a moving sun, with real-time shadows and
cinematic post-processing. This code is raw, untested and thus should
not be used in anything serious.

Features:

* First-person walk / fly camera with mouse look
* Sun with day movement and shadow-casting light
* Heightmapped terrain with moss / earth / rock texturing
* Hundreds of instanced trees, plants and rocks
* Sky with fog-blended horizon
* Post effects: bloom, god rays, lens flares, FXAA
* Adaptive exposure with filmic tonemapping
* Ambient audio

	Copyright (c) 2012 Thibaut Despoulain, Lauranne Didierjean, Baptiste Aubry, Adrien Mauranyapin.

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
