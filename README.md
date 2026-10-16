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

Look around - hold left mouse button, or F1 (mouse capture toggle)

Effects - 1 (FXAA), 2 (lens flares), 3 (auto-exposure),
4 (god rays), 5 (bloom), 6 (distortion)

Exposure - 7 / 8 (bias down / up)

Debug - F3 (FPS overlay), F4 (uncapped FPS), R (regenerate vegetation),
9 (shadows on/off, diagnostic), 0 (wireframe, diagnostic)

Quit - Escape

## Renderer

Linear HDR pipeline: lit shaders work in linear (sRGB inputs decoded
on sample), shadows are 2-cascade PCF from 24-bit depth maps, and the
final display pass applies auto-exposure (log-average luminance with
temporal adaptation), ACES tonemapping and gamma 2.2. Per-frame
exposure diagnostics are logged as `autoexposure ...` lines.

See [OriginalReadme.md](OriginalReadme.md) for the original project readme.
