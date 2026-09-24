# ft_vox
A simple voxel engine built with modern C++ and Vulkan. The project is designed to be a learning experience for graphics programming and engine development, with a focus on clean code and modular design.

**WORK IN PROGRESS**

![WIP image of ft_vox](https://github.com/xwojtuss/scop/blob/6d7157b6b6f3806dffff252d198427a8c6543e07/ft_vox/WIP.png)

## Prerequisites

- C++17 compatible compiler
- CMake 3.25 or later
- Vulkan SDK
- GLFW (auto-fetched)
- GLM (auto-fetched)
- ImGui (auto-fetched)
- stb_image (single header, included)
- tinyobjloader (single header, included)
- nlohmann/json (single header, included)
- Catch2 v3 (auto-fetched, tests only)
- gcovr (coverage only): `python3 -m pip install --user gcovr`

## Getting Started
After cloning the repository you can build the project using cmake:

```bash
cmake --preset dev-clang
cmake --build --preset dev-clang
```

Then run the executable:
```bash
./build/dev-clang/scop
```

## Tests

Tests use [Catch2 v3](https://github.com/catchorg/Catch2) BDD-style and live in `tests/`, mirroring `srcs/`. Any `*.cpp` added there is picked up automatically.

Build and run all tests:
```bash
cmake --workflow --preset test
```

Run only some scenarios by tag, e.g. `[chunk]`:
```bash
./build/dev-clang/tests/scop_tests "[chunk]"
```

Run the tests with a coverage report (fails if line coverage is below 80%):
```bash
cmake --workflow --preset coverage
```

The HTML report is written to `build/coverage/coverage/index.html`. What counts towards coverage (and the 80% threshold) is configured in `gcovr.cfg`. On 42 workstations use the `test42` and `coverage42` presets instead.

## Documentation

To generate documentation (might take a while):
```bash
cmake --build --preset dev-clang --target generate_docs
```

## Author
* **Wojtek Kornatowski** - [xwojtuss](https://github.com/xwojtuss)
