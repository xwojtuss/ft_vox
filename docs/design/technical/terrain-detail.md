# Terrain detail

Distant terrain works like the Minecraft mod [Voxy](https://github.com/MCRcortex/voxy), not like Distant Horizons.

- **Real blocks at every distance.** Terrain is kept at several levels of detail. Each level up, one cell covers twice as many blocks along every axis and holds one real block chosen from the cells below it, never an averaged color. Distant terrain keeps its overhangs, caves and textures.
- **Detail follows size on screen.** The GPU walks the detail tree every frame and draws a section as it is once it is small enough on screen. A distant mountain gets more detail than a flat plain at the same distance.
- **One detail tree for all terrain.** Near chunks are level 0 of the same tree that draws distant terrain, so every level shares culling and streaming. Near terrain keeps the full mesher with special models, transparency and liquids. Decided in [#167](https://github.com/xwojtuss/ft_vox/issues/167).
- **Distant terrain is generated and downsampled.** Terrain beyond the explored area is generated in the background, downsampled into the detail levels and then discarded, so a fresh world already has a horizon. Decided in [#168](https://github.com/xwojtuss/ft_vox/issues/168).
