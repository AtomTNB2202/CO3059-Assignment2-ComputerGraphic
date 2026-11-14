# Study and demonstration guide

The console foundation has grown into the final application. Read the modules in
this order rather than trying to learn the complete project at once.

1. **Window and lifecycle:** `src/main.cpp` initializes GLFW, requests a 3.3 core
   context, loads GLAD, initializes ImGui, and shuts down in reverse order.
2. **The render loop:** `src/core/Application.cpp` polls input, creates GUI widgets,
   draws the scene, draws ImGui on top, and swaps buffers.
3. **Vertex data:** `include/Geometry.h` defines position, normal, and UV for each
   vertex. `src/graphics/Mesh.cpp` uploads these to a VBO and an EBO, records the
   attribute layout in a VAO, and calls `glDrawElements`.
4. **Shaders:** `src/core/Shader.cpp` reads GLSL files, reports compile/link errors,
   and sets uniforms. Start by following `basic.vert` and the flat-color branch
   in `basic.frag`.
5. **Transforms:** `Transform::matrix()` composes `T * Rz * Ry * Rx * S`. The vertex
   shader applies `projection * view * model * position`. Rotations are degrees
   in the GUI and converted to radians for GLM.
6. **Camera:** `Camera.cpp` derives a camera position from yaw, pitch, and distance.
   `lookAt` builds the view matrix. Zoom changes distance, and pan moves the target
   along the camera's right and up directions. WASD translates the camera and
   target together; Q/E moves vertically. Middle/Shift-left dragging translates
   only the selected object in a view-parallel plane at that object's depth.
7. **Lighting:** Gouraud lighting runs at vertices and interpolates the resulting
   color. Phong lighting interpolates world position and normal, then computes
   ambient, diffuse, and specular terms per fragment. The inverse-transpose normal
   matrix handles non-uniform scaling.
8. **Shapes:** `Geometry.cpp` generates curves with sine/cosine, triangulates
   concave 2D shapes with ear clipping, and builds indexed surface grids.
9. **Import and textures:** `ModelImport.cpp` loads OBJ geometry, supplies missing
   normals, and centers/rescales models. `Texture.cpp` decodes image bytes and
   uploads a 2D texture with mipmaps.
10. **Scene and GUI:** `Scene` owns objects and stable IDs. The inspector edits the
    selected object's data. GUI input capture prevents camera motion while editing.
11. **Atoms and molecules:** `Molecule.cpp` supplies element data and initial
    molecule positions. The renderer reuses sphere/cylinder meshes and combines
    each local transform with its parent object's transform. The bond rotation
    aligns the cylinder's Y axis to the vector between the two atoms.
12. **Surface expressions:** `Expression.cpp` is a recursive-descent parser with
    explicit arithmetic precedence. It builds callable expressions; it does not
    evaluate operating-system commands or C++ source.
13. **Picking and gizmos:** `Renderer.cpp` draws IDs into an offscreen color/depth
    target. A click reads the visible object's ID; `outline.frag` marks its border.
    `Editor.cpp` integrates ImGuizmo's world-axis arrows and rings, preserves scale,
    and gives active handles priority over picking and camera input.
14. **Scene files:** `SceneIO.cpp` defines a versioned JSON schema using
    nlohmann/json. It rebuilds meshes/resources into a temporary document before
    replacing the active scene. `FileDialog.cpp` wraps Windows Unicode dialogs.
15. **Focus and animation:** `Bounds.cpp` transforms local bounds into world bounds;
    `Camera::frame` fits a sphere using the narrower viewport field of view.
    `Animation.h` advances a playback clock; the renderer offsets electron angles.

## Suggested in-class demonstration

1. Start the app and rotate the default cube with a right drag.
2. Change its position, rotation, and scale; use Reset transform afterward.
3. Switch through Flat, Gouraud, Phong, Texture, and Wireframe.
4. Clear the scene, open Example galleries, and add the 2D and 3D galleries
   separately. Use Front / 2D for planar objects.
5. Add `sin(x)*cos(y)`, then `0.2*(x^2-y^2)` as mathematical surfaces. Try `sqrt(-1)`
   to demonstrate an error without losing the existing scene.
6. Import the bundled pyramid OBJ. Choose Texture mode and load a PNG or JPG.
7. Clear the scene and add hydrogen, helium, and oxygen Bohr models separately.
8. Add H2O and CO2. Change an atom's type and position; observe the bond following it.
9. Save a screenshot and open the resulting PNG next to the executable.
10. Click a visible object, drag each colored arrow in Move mode, then select
    Rotate and drag a ring. Press F to fit a rotated/scaled object.
11. Play/pause oxygen's electrons and adjust Orbit speed.
12. Save scene, change the scene, then Open scene and confirm replacement to restore it.

## Manual checks before submission

- Orbit, Shift-right pan, zoom, reset, and focus work; dragging a GUI field does not orbit.
- Hover the scene and use WASD/QE; type those letters in a formula and confirm
  that the camera does not move.
- Move the camera away from the origin and add each object type; its origin
  should align with the center cross.
- Select one of two objects and middle/Shift-left drag; only that object moves.
- Click overlapping objects: the frontmost visible surface wins. Click empty space
  to deselect; hidden objects and grid lines must not intercept selection.
- Drag X/Y/Z arrows, then X/Y/Z rotation rings. Only the selected object changes;
  the camera and nonuniform scale stay fixed. Orbit to expose foreshortened handles.
- Grid toggling and the XY grid in Front / 2D work without affecting objects.
- Selection, visibility, deletion, and clear-scene confirmation work.
- Every shape has sensible surfaces under Phong and wireframe modes.
- Texture and OBJ paths with spaces work; invalid paths show an error.
- Use Browse for OBJ and PNG/JPG, including a Unicode path. Test dialog Cancel.
- Save and reopen a scene with a surface, textured OBJ, edited molecule, and camera
  position. Test canceling replacement, malformed JSON, and a missing external asset.
- Press F on large, small, nonuniformly scaled objects and an edited molecule;
  the complete object should fit. Test a tall/narrow viewport too.
- Play electrons, change speed, pause, and resume. Camera controls still work.
- Resize the window and minimize/restore it without stretching or crashing.
- Save, close, relaunch, and Open scene to restore the saved scene.
- Read the report and explain the shader and transformation code in your own words.

The automated tests cover geometry and rendering, but do not replace a full
hands-on interaction review on the computer used for the demonstration.
