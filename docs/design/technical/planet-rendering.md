# Planet rendering

## Decision

Every planet is a **flat square map that loops in both directions**. It turns into a sphere **only visually** as the player climbs, so players can fly around a planet and on to other planets with no loading cut, while gameplay stays on a simple flat grid. Decided in [#72](https://github.com/xwojtuss/ft_vox/issues/72).

## What it requires

- **A vertex shader that bends the world by altitude:** flat on the ground, a full sphere in orbit.
- **A sphere always centred on the point under the camera,** so the distorted seam where the map's edges meet stays hidden on the far side of the planet.
- **Chunk coordinates that wrap around,** for the looping map.
- **Low-detail terrain** for distant views and the view from orbit.
- **Coordinates for space:** a floating origin, plus a smooth switch to solar-system coordinates far from a planet.

## Known limitations

- The far side of the sphere is geometrically wrong, but never visible.
- Long loops in orbit can leave the player slightly rotated compared to the flat map; rarely noticeable.
- The sphere's circumference equals the map width, so small maps give small-looking planets.

## Next step

Prototype the bending shader in the current renderer before building anything else, to check it looks and feels right.
