# Original tabletop demo assets

These simple meshes are authored by `scripts/create_tabletop_assets.py`, using
only Python's standard library. No third-party mesh downloads are included.
The application reads the OBJ files through `loadObj()` and tinyobjloader.

- `table.obj`: tabletop and four legs.
- `cup.obj`: hollow cylindrical body, bottom, rim and a torus handle.
- `book.obj`: thin rectangular solid.
- `ball.obj`: latitude/longitude sphere.

UV coordinates are included; the importer generates normals and normalizes each
model into a two-unit box. The scene `assets/scenes/tabletop.json` accounts for
this normalization and places 2 cups, 1 book and 1 ball on the table.
The table transform is pinned during batch randomization.

To regenerate these assets and their demo scene/config from the project root:

```powershell
python scripts/create_tabletop_assets.py
```

This overwrites the generated demo asset/scene/config filenames. It is not needed
for building or running the application; all generated files are already included.
