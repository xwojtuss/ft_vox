# Blocks

- **One block per block space.** Nothing shares a space with another block. Cables and pipes (if they will exist) are blocks of their own. Decided in [#91](https://github.com/xwojtuss/ft_vox/issues/91).
- **Any block can take any generic shape.** A block such as dirt is defined once and can be placed as a cube or another generic shape, in any orientation, like hammered tiles in Terraria. Shape and orientation belong to the placed block. Decided in [#92](https://github.com/xwojtuss/ft_vox/issues/92).
- **Special blocks have their own models.** Doors, furniture and plants use a model instead of a generic shape. They are rare, so the world is built almost entirely from generic shapes.
- **A special block can span several block spaces,** like a door two spaces tall or a table three spaces wide. It still counts as the one block in each space it covers, so nothing else can be placed there.
- **Liquids share a block space with a block.** Each block space holds one block and optionally one liquid with a fill level. Blocks decide whether a liquid can enter: stone keeps water out, a fence or a slab lets it in. Decided in [#93](https://github.com/xwojtuss/ft_vox/issues/93).
- **Gases are not stored per block space.** They belong to a separate atmosphere system.
