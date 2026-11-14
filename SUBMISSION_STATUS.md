# Assignment 2 source package status

This is a draft source/evidence package. It is not a complete final submission.

Implemented and checked: OBJ scene import, manual configuration, clean RGB,
instance ID maps and binary masks, visible bounding boxes, category labels,
COCO compressed RLE, float32 numerical depth, metadata, deterministic seeded
batch generation and replay scenes. The user confirmed the milestone 0-4 GUI flow.

Still required before final submission:

- Assignment 2 final report in English, both LaTeX `.tex` and compiled `.pdf`.
  The Assignment 1 report from the original checkout is not included here.
- Use the confirmed Assignment 2 author in the report: Nguyễn Vinh Anh Quân — 2353007.
  The selected public repository is https://github.com/AtomTNB2202/CO3059-Assignment2-ComputerGraphic.
- The implementation selects COCO under section 2.1's "COCO or YOLO" wording;
  section 2.2 also lists both formats. YOLO is not implemented in this version.
  Do not describe YOLO export as implemented in a report/demo.
- THIRD_PARTY.md documents GLAD, Dear ImGui, ImGuizmo, stb, tinyobjloader and
  nlohmann/json as support libraries; the OpenGL/GLSL and annotation pipeline is
  implemented in the project. This is not an instructor approval statement.
- Rehearse the demo and follow the course LMS submission instructions.

Build/run: see README.md. Rubric mapping, evidence and demo: see docs/MILESTONE5_VI.md.
Optional Python verification tools are separate from the C++ renderer/exporter.
