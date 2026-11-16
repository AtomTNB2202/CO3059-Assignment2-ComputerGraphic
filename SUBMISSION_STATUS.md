# Assignment 2 source package status

This working tree contains source, evidence and the Assignment 2 report.
It has not been submitted to the LMS.

Implemented and checked: OBJ scene import, manual configuration, clean RGB,
instance ID maps and binary masks, visible bounding boxes, category labels,
COCO compressed RLE, float32 numerical depth, metadata, deterministic seeded
batch generation and replay scenes. The user confirmed the milestone 0-4 GUI flow.

Current report status (November 16, 2026):

- Assignment 2 English report is now in `report/report.tex` and `report/report.pdf`,
  with required images in `report/figures/`. Build instructions: `report/README.md`.
- Use the confirmed Assignment 2 author in the report: Nguyễn Vinh Anh Quân — 2353007.
  The selected public repository is https://github.com/AtomTNB2202/CO3059-Assignment2-ComputerGraphic.
- COCO and YOLO detection export are implemented. The report also covers the
  export preview, cursor depth readout and saved-dataset viewer. Older progress
  documents may predate these features.
- THIRD_PARTY.md documents GLAD, Dear ImGui, ImGuizmo, stb, tinyobjloader and
  nlohmann/json as support libraries; the OpenGL/GLSL and annotation pipeline is
  implemented in the project. This is not an instructor approval statement.
- Rehearse the demo and follow the course LMS submission instructions.

Before submission, review the report, commit/push the intended changes and prepare
the source package. The existing draft packaging script excludes `report/`;
attach TEX, PDF and figures separately. No final archive or LMS upload is implied.

Build/run: see README.md. Rubric mapping, evidence and demo: see docs/MILESTONE5_VI.md.
Optional Python verification tools are separate from the C++ renderer/exporter.
