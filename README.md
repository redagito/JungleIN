# Refactor of the JungleIN project

Modernized OpenGL jungle viewer: `App` holds the executable entry point,
`Lib` the engine as a static library, `Test` the Catch2 test suite.

## Requirements

Development is done with
* Conan 2.32.0
* CMake 4.4.3 (minimum CMake 3.28)
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

Quit - Escape

# Original Readme
JungleIN
========
![Splash](http://bkcore.com/orion/uploads/images/teaser.junglein.jpg)

[Web page of the project](http://bkcore.com/blog/3d/opengl-jungle-viewer-source-utbm.html)
[Youtube demo video](http://youtu.be/4M-wfzuCgvs)

This is the undocumented source code of an OpenGL jungle viewer written as part of a scholar project.

This code is raw, untested and thus should not be used in anything serious. 


	Copyright (c) 2012 Thibaut Despoulain, Lauranne Didierjean, Baptiste Aubry, Adrien Mauranyapin.

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

![Screen](http://bkcore.com/orion/uploads/images/junglein/bloom.jpg)
![Screen](http://bkcore.com/orion/uploads/images/junglein/shadows.jpg)
