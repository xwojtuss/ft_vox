# Profiling

The game uses [Tracy](https://github.com/wolfpld/tracy) (pinned to v0.13.1 in `CMakeLists.txt`). The profiler is off by default, and every profiling macro compiles to nothing then.

## Build and run

```bash
cmake --preset profile
cmake --build --preset profile
./build/profile/ft_vox
```

This is a Release build with `FT_VOX_PROFILER=ON`, the one to take timings from. For working on the profiling zones themselves, use the Debug variant (`dev-profile`, binary in `build/dev-profile/`): it keeps the validation layers and debug symbols, but its timings are not representative.

The game records from startup and listens on port 8086 until a viewer connects.

## Viewer

The Tracy viewer must be exactly the same version as the library (v0.13.1). Download or build it from the matching release of the Tracy repository, start it and press Connect on `127.0.0.1`.

## Marking code

Include `profiling/Profiler.hpp` and use the macros. Game code never includes Tracy directly.

| Macro | Use |
| --- | --- |
| `FT_PROFILE_FRAME()` | Marks the end of a frame. Called once in the main loop. |
| `FT_PROFILE_FUNCTION()` | Zone named after the enclosing function, until the end of the scope. |
| `FT_PROFILE_ZONE("name")` | Named zone until the end of the scope. The name must be a string literal. |
| `FT_PROFILE_THREAD("name")` | Names the calling thread. Call it first thing in every worker thread. |
| `FT_PROFILE_GPU_ZONE(profiler, commandBuffer, "name")` | GPU zone recorded into the command buffer. The renderer owns the `profiling::GpuProfiler`. |

Main thread zones: Input, Simulate, Update, chunk generation, meshing, and the renderer steps. GPU time for drawing chunks is the `Draw chunks` zone.
