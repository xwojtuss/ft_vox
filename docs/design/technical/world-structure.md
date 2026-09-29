# World structure

Decided in [#68](https://github.com/xwojtuss/ft_vox/issues/68).

- **World:** what players create, name, seed and save. It holds everything below, and later possibly several star systems (DLC).
- **Star system:** the three suns on fixed paths, the eras and the Debris Belt.
- **Planet:** one looping flat map with its own terrain and coordinates.
- **Space:** everything between planets, with its own coordinates.
