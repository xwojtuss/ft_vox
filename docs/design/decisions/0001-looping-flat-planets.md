# 0001: Planets are looping flat maps shown as spheres

- **Date:** 2026-09-28
- **Status:** Accepted

## Decision

Each planet is a flat square map that loops in both directions. It is rendered as a sphere only visually as the player climbs; gameplay stays on a flat grid. Details: [technical/planet-rendering.md](../technical/planet-rendering.md).

## Why

- Travel from the ground to orbit and other planets without any loading cut.
- The planet is finite but has no walls: walk in any direction and you come back.
- Gameplay, building, chunks and world generation stay simple.

## Rejected alternatives

- **Real spherical voxel planets:** far too complex (curved chunks, gravity, building).
- **Cube planets:** awkward corners, and rounding the edges would complicate world generation.
- **Flat looping world with a hidden cut to a separate sphere:** a visible or hidden seam instead of a smooth transformation.
- **Floating slab worlds:** no looping, visible world edge.
