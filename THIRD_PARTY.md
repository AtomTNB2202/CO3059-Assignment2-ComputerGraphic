# Dependencies and asset provenance

## COCO format compatibility and optional verification

The C++ exporter implements the COCO compressed RLE data format independently;
it does not link the COCO API or invoke a dataset generator. Format references:
[COCO mask interface](https://github.com/cocodataset/cocoapi/blob/master/PythonAPI/pycocotools/mask.py)
and [compressed encoding specification in the reference implementation](https://github.com/cocodataset/cocoapi/blob/master/common/maskApi.c).
Optional `scripts/verify_coco.py` uses Python numpy, Pillow and official
pycocotools to decode exported annotations and compare them with the saved PNGs.
These packages are only for verification and are not C++ runtime/build dependencies.

Depth `.npy` output is written directly in C++ according to the
[NumPy NPY 1.0 specification](https://numpy.org/doc/stable/reference/generated/numpy.lib.format.html).
No NumPy library is linked by the application. Optional `scripts/inspect_depth.py`
uses numpy and Pillow to read saved numerical depth and verify PNG alignment.

CMake downloads pinned archives and verifies their SHA-256 hashes. The first
configure requires internet access; subsequent builds reuse the local downloads.

| Dependency | Version | Purpose | License |
|---|---|---|---|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Window, context, input | zlib/libpng |
| [GLAD](https://github.com/Dav1dde/glad) | 2.0.8 | OpenGL function loader | MIT / generated-code terms |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Vectors and matrices | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.9b | GUI, GLFW/OpenGL3 backends | MIT |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | `18cef5e031d8c6973d80284c67f60549fafd78c1` | Translation arrows and rotation rings; handle hit testing and manipulation | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON encoding/decoding for scene files | MIT |
| [stb](https://github.com/nothings/stb) | `2c980bb59875b0d32144a71867fbdebb2f77cd20` | PNG/JPG decoding and PNG writing | MIT or public domain |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | 2.0.0rc13 | OBJ triangulation and parsing | MIT |

The dependency archives include their original license files in the build
directory's `_deps` folder. GLAD's license is included in `third_party/glad/LICENSE`.
The bundled loader was generated with:

```text
python -m glad --api gl:core=3.3 --extensions GL_KHR_debug --out-path third_party/glad --reproducible c
```

Python and GLAD's generator are not required to build the project because the
generated C source and headers are included. No deprecated OpenGL rendering
functions are used by the application.

The checker textures and pyramid OBJ are original procedural/sample assets
created for this project. Screenshots are captured from the application itself.
The table, cup, book and ball OBJ files in `assets/models/tabletop/` are also
original procedural demo assets. Their standard-library-only authoring script is
`scripts/create_tabletop_assets.py`. The application loads the resulting files
through the existing OBJ importer; Python is not required for rendering them.
Windows Segoe UI is loaded from the operating system if available and is not
redistributed; Dear ImGui's default font is the fallback.

Windows file browsing uses the operating system's `GetOpenFileNameW` and
`GetSaveFileNameW` through `comdlg32`; no additional file-dialog library is bundled.
The application implements its own ID-buffer object picking, outline shader,
camera framing, scene schema/validation, asset reconstruction, and electron orbit
updates. ImGuizmo supplies the manipulation widget; nlohmann/json supplies JSON
syntax handling, not the application's scene model.
