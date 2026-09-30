# Planet generation

- **Planets are built from reusable pieces.** Terrain shape, water, biomes and caves are pieces that any planet can use. A planet picks its pieces and sets their parameters in its planet file. When a planet needs something new, it becomes a new piece that other planets can use too.
- **Every planet sets its own size.** Planets can be small or large, and a planet's size is part of its planet file.
- **Planets are planned with a planet-wide atlas.** When a planet is created, a coarse atlas of the whole planet is computed first: elevation, water flow and climate, one cell per few blocks. Chunks are generated later, when players come near, and add local detail to what the atlas says. This lets rivers flow downhill all the way to the sea, and lets the whole planet be drawn from orbit without generating any chunk. Decided in [#134](https://github.com/xwojtuss/ft_vox/issues/134).
