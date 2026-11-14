#include "Application.h"
#include "SceneIO.h"
#include "Molecule.h"
#include "Depth.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <chrono>

namespace
{
void check(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void reject(const std::function<void()> &operation)
{
    bool failed = false;
    try
    {
        operation();
    }
    catch (const std::exception &)
    {
        failed = true;
    }
    check(failed, "Invalid scene was accepted.");
}
} // namespace

// Runs in the hidden-window smoke test, with a real GL context for resource round trips and picking.
void Application::verifyEditor()
{
    SceneDocument document;
    for (int i = 0; i < 17; ++i)
    {
        Object object;
        object.name = shapeNames[i];
        object.shape = static_cast<Shape>(i);
        object.mesh = std::make_shared<Mesh>(makeShape(object.shape));
        object.transform.position = {float(i), 1, -2};
        object.transform.rotation = {25, 35, 45};
        object.transform.scale = {0.5f, 2, 1.5f};
        object.material.mode = static_cast<RenderMode>(i % 5);
        document.scene.add(std::move(object));
    }
    Object surface;
    surface.kind = ObjectKind::Surface;
    surface.surfaceExpression = "0.2*(x^2-y^2)";
    surface.surfaceExtent = 3;
    surface.surfaceResolution = 24;
    surface.mesh = std::make_shared<Mesh>(makeSurface(surface.surfaceExpression, 3, 24));
    document.scene.add(std::move(surface));
    Object model;
    model.kind = ObjectKind::Imported;
    model.modelPath = (root / "assets/models/pyramid.obj").u8string();
    model.mesh = std::make_shared<Mesh>(loadObj(model.modelPath));
    model.material.texturePath = (root / "assets/textures/checker.jpg").u8string();
    model.material.texture = std::make_shared<Texture>(std::filesystem::u8path(model.material.texturePath));
    model.material.mode = RenderMode::Texture;
    document.scene.add(std::move(model));
    document.scene.add(makeAtom(4));
    auto water = makeMolecule(true);
    water.atoms[1].position = {-2, 1, 0.3f};
    water.atoms[2].element = 1;
    document.scene.add(std::move(water));
    document.scene.add(makeMolecule(false));
    document.scene.objects.front().visible = false;
    document.camera.target = {2, 3, -4};
    document.camera.yaw = 65;
    document.camera.distance = 19;
    document.animation.playing = true;
    document.animation.time = 2.5;
    document.animation.speed = 0.5f;
    document.background = {0.1f, 0.2f, 0.3f};
    document.lightPosition = {-3, 4, 8};
    document.showGrid = false;
    document.scene.objects[18].categoryId = 77;
    document.scene.objects[18].categoryName = "custom_pyramid";
    document.scene.objects[18].id = 500;
    document.scene.nextId = 501;
    document.scene.objects[0].randomizeTransform = false;
    auto folder = root / "test-artifacts";
    std::filesystem::create_directories(folder);
    auto path = folder / "editor-scene.json";
    saveScene(path, document);
    auto restored = loadScene(path);
    check(restored.scene.objects.size() == document.scene.objects.size(), "Scene round-trip lost objects.");
    for (size_t i = 0; i < document.scene.objects.size(); ++i)
    {
        auto &a = document.scene.objects[i], &b = restored.scene.objects[i];
        check(a.kind == b.kind && a.shape == b.shape && a.visible == b.visible && a.name == b.name,
              "Object metadata changed.");
        check(glm::length(a.transform.position - b.transform.position) < 1e-5f &&
                  glm::length(a.transform.rotation - b.transform.rotation) < 1e-5f &&
                  glm::length(a.transform.scale - b.transform.scale) < 1e-5f,
              "Transform changed on reload.");
        check(a.material.mode == b.material.mode, "Render mode changed on reload.");
        check(a.categoryId == b.categoryId && a.categoryName == b.categoryName,
              "Category label changed on reload.");
        check(a.id == b.id && a.randomizeTransform == b.randomizeTransform,
              "Saved instance ID or batch transform lock changed on reload.");
        if (a.mesh)
            check(a.mesh->triangleCount() == b.mesh->triangleCount(), "Mesh was rebuilt incorrectly.");
    }
    check(restored.scene.objects[18].material.texture != nullptr, "Texture not restored.");
    check(restored.scene.nextId == 501, "Scene next instance ID drifted on reload.");
    BatchSettings variation;
    variation.count = 3; variation.textures = false;
    auto sampleA = makeBatchSample(document, variation, 0);
    auto sampleB = makeBatchSample(document, variation, 0);
    auto sampleC = makeBatchSample(document, variation, 1);
    check(sampleA.camera.yaw == sampleB.camera.yaw && sampleA.camera.yaw != sampleC.camera.yaw,
          "Batch seed/index is not reproducible or samples do not vary.");
    for (size_t i = 0; i < document.scene.objects.size(); ++i)
    {
        const auto &baseObject = document.scene.objects[i];
        const auto &a = sampleA.scene.objects[i];
        const auto &b = sampleB.scene.objects[i];
        check(glm::length(a.transform.position - b.transform.position) < 1e-6f &&
              glm::length(a.material.color - b.material.color) < 1e-6f &&
              std::abs(objectBounds(a).low.y - objectBounds(baseObject).low.y) < 1e-4f,
              "Batch sample is not repeatable or changed base support height.");
        check(a.id == baseObject.id && a.categoryId == baseObject.categoryId,
              "Batch changed instance/category mapping.");
    }
    check(glm::length(sampleA.scene.objects[0].transform.position - document.scene.objects[0].transform.position) < 1e-6f &&
          glm::length(sampleA.scene.objects[0].transform.scale - document.scene.objects[0].transform.scale) < 1e-6f,
          "Pinned transform was randomized.");
    reject([&] { auto bad = variation; bad.count = 0; makeBatchSample(document, bad, 0); });
    reject([&] { auto bad = variation; bad.scaleJitter = 2; makeBatchSample(document, bad, 0); });
    // Exercise disk export, category merge, and editor state restoration at small resolution.
    auto beforeBatchScene = scene;
    auto beforeBatchCamera = camera;
    auto beforeBatchBackground = renderer.background;
    variation.count = 2; variation.width = 97; variation.height = 73;
    auto batchPath = folder / ("batch-smoke-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    exportBatch(batchPath, variation);
    check(scene.selected == beforeBatchScene.selected && scene.nextId == beforeBatchScene.nextId &&
          camera.yaw == beforeBatchCamera.yaw && camera.distance == beforeBatchCamera.distance &&
          glm::length(renderer.background - beforeBatchBackground) < 1e-6f &&
          glm::length(scene.objects.front().transform.position - beforeBatchScene.objects.front().transform.position) < 1e-6f,
          "Batch export changed the editor scene.");
    nlohmann::json mergedBatch;
    { std::ifstream file(batchPath / "annotations.json"); file >> mergedBatch; }
    check(mergedBatch["images"].size() == 2 && mergedBatch["categories"].size() == 1 &&
          std::filesystem::exists(batchPath / "sample_000002/depth.npy"),
          "Batch export lost images/categories or depth data.");
    reject([&] { exportBatch(batchPath, variation); });
    std::cout << "BATCH PASS: seed/index, pinned transforms, support height, stable IDs, COCO merge, editor restoration.\n";
    check(restored.scene.objects[20].atoms[2].element == 1, "Edited molecular atom type lost.");
    check(glm::length(restored.scene.objects[20].atoms[1].position - glm::vec3(-2, 1, 0.3f)) < 1e-5f,
          "Edited molecular atom position lost.");
    check(glm::length(restored.camera.target - document.camera.target) < 1e-5f &&
              restored.camera.distance == 19 && !restored.showGrid && restored.animation.playing &&
              restored.animation.time == 2.5,
          "View/animation state lost.");
    document.scene.objects[1].name = "Overwrite test";
    saveScene(path, document);
    check(loadScene(path).scene.objects[1].name == "Overwrite test", "Replacing saved scene failed.");
    nlohmann::json original;
    {
        std::ifstream file(path);
        file >> original;
    }
    auto badPath = folder / "invalid-scene.json";
    auto invalidCategory = document;
    invalidCategory.scene.objects[0].categoryName.clear();
    reject([&] { saveScene(badPath, invalidCategory); });
    auto legacy = original;
    for (auto &object : legacy["objects"])
    {
        object.erase("category_id");
        object.erase("category_name");
    }
    auto legacyPath = folder / "legacy-scene.json";
    {
        std::ofstream file(legacyPath);
        file << legacy.dump(2);
    }
    auto legacyScene = loadScene(legacyPath);
    check(legacyScene.scene.objects[18].categoryName == "pyramid" &&
          legacyScene.scene.objects[18].categoryId > 0, "Legacy OBJ category was not inferred.");
    auto pyramid = legacyScene.scene.objects[18];
    int pyramidCategory = pyramid.categoryId;
    pyramid.categoryId = 0;
    legacyScene.scene.add(pyramid);
    check(legacyScene.scene.objects.back().categoryId == pyramidCategory &&
          legacyScene.scene.objects.back().id != legacyScene.scene.objects[18].id,
          "Same model should share category but have a different instance ID.");
    for (int failure = 0; failure < 6; ++failure)
    {
        auto bad = original;
        if (failure == 0)
            bad["version"] = 99;
        if (failure == 1)
            bad["objects"][0]["scale"] = {0, 1, 1};
        if (failure == 2)
            bad["objects"][0]["shape"] = 999;
        if (failure == 3)
            bad["objects"][18]["model"] = "missing-model.obj";
        if (failure == 4)
            bad["objects"][18]["material"]["texture"] = "missing-image.png";
        if (failure == 5)
            bad["objects"][20]["bonds"][0] = {0, 999};
        {
            std::ofstream file(badPath);
            file << bad.dump();
        }
        reject([&] { loadScene(badPath); });
    }
    {
        std::ofstream file(badPath);
        file << "{not valid JSON";
    }
    auto originalCount = scene.objects.size();
    auto originalSelection = scene.selected;
    restoreSceneFile(badPath);
    check(scene.objects.size() == originalCount && scene.selected == originalSelection,
          "Failed scene load damaged current scene.");

    // Matrices from the rotation gizmo must match our Rz * Ry * Rx convention.
    for (glm::vec3 rotation : {glm::vec3(25, 35, 45), glm::vec3(0, 90, 0), glm::vec3(-70, 15, 140)})
    {
        Transform initial;
        initial.position = {2, 3, 4};
        initial.rotation = rotation;
        auto matrix = initial.matrix();
        glm::vec3 p, r, s;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), glm::value_ptr(p), glm::value_ptr(r),
                                              glm::value_ptr(s));
        Transform reconstructed;
        reconstructed.position = p;
        reconstructed.rotation = r;
        auto result = reconstructed.matrix();
        for (int column = 0; column < 4; ++column)
            check(glm::length(matrix[column] - result[column]) < 1e-3f,
                  "Gizmo rotation convention mismatch.");
    }
    for (float aspect : {0.4f, 1.0f, 2.5f})
    {
        auto bounds = objectBounds(document.scene.objects[9]);
        Camera framed;
        framed.frame(bounds.low, bounds.high, aspect);
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 p{i & 1 ? bounds.high.x : bounds.low.x, i & 2 ? bounds.high.y : bounds.low.y,
                        i & 4 ? bounds.high.z : bounds.low.z};
            auto clip = framed.projection(aspect) * framed.view() * glm::vec4(p, 1);
            check(clip.w > 0 && std::abs(clip.x / clip.w) < 1 && std::abs(clip.y / clip.w) < 1,
                  "Focus does not fit all transformed bounds.");
        }
    }
    Scene pickScene;
    Camera pickCamera;
    pickCamera.front();
    Object far;
    far.mesh = std::make_shared<Mesh>(makeShape(Shape::Cube));
    far.transform.position.z = -3;
    pickScene.add(far);
    int farId = pickScene.selected;
    Object near;
    near.mesh = far.mesh;
    pickScene.add(near);
    int nearId = pickScene.selected;
    // Odd width exercises RGB pack alignment; off-center objects catch vertical flips.
    Scene datasetScene = pickScene;
    datasetScene.objects[0].transform.position = {-0.4f, 0.5f, -3};
    datasetScene.objects[1].transform.position = {0.4f, 0.5f, 0};
    datasetScene.objects[1].id = 513;
    datasetScene.objects[0].material.mode = RenderMode::Flat;
    datasetScene.objects[1].material.mode = RenderMode::Flat;
    datasetScene.objects[0].material.color = {0, 1, 0};
    datasetScene.objects[1].material.color = {1, 0, 0};
    auto captured = renderer.captureDataset(datasetScene, pickCamera, 321, 241);
    size_t green = 0, red = 0, upper = 0, lower = 0;
    for (size_t p = 0; p < captured.rgb.size() / 3; ++p)
    {
        int id = captured.instanceIds[3*p] | (captured.instanceIds[3*p+1] << 8) |
                 (captured.instanceIds[3*p+2] << 16);
        if (!id) continue;
        check(id == farId || id == 513, "Capture produced an unknown instance ID.");
        check(captured.rgb[3*p] == (id == 513 ? 255 : 0) &&
              captured.rgb[3*p+1] == (id == farId ? 255 : 0),
              "RGB and instance mask disagree at a pixel.");
        if (id == farId) ++green; else ++red;
        if (p / 321 < 120) ++upper; else ++lower;
    }
    check(green > 0 && red > 0 && upper > lower, "Capture lost instances or flipped vertically.");
    std::reverse(datasetScene.objects.begin(), datasetScene.objects.end());
    auto reversed = renderer.captureDataset(datasetScene, pickCamera, 321, 241);
    check(captured.instanceIds == reversed.instanceIds && captured.rgb == reversed.rgb,
          "Dataset occlusion depends on draw order.");
    datasetScene.selected = -1;
    bool gridBefore = renderer.showGrid;
    renderer.showGrid = !gridBefore;
    auto clean = renderer.captureDataset(datasetScene, pickCamera, 321, 241);
    check(clean.rgb == captured.rgb && clean.instanceIds == captured.instanceIds,
          "Editor selection/grid contaminated dataset export.");
    check(renderer.showGrid == !gridBefore, "Capture failed to restore grid setting.");
    renderer.showGrid = gridBefore;
    check(captured.depth == reversed.depth && captured.depth == clean.depth,
          "Depth changed with draw order or editor settings.");
    for (size_t p = 0; p < captured.depth.size(); ++p)
    {
        bool foreground = captured.instanceIds[3*p] || captured.instanceIds[3*p+1] || captured.instanceIds[3*p+2];
        check(foreground ? captured.depth[p] > 0 : captured.depth[p] == 0,
              "Depth background and ID map disagree.");
    }
    // Two front-facing planes at known camera-forward distances, independent of inverse formula.
    Scene depthScene;
    Object plane;
    plane.shape = Shape::Rectangle;
    plane.mesh = std::make_shared<Mesh>(makeShape(plane.shape));
    plane.transform.position = {-0.35f, 0.65f, -3}; // Camera at z=7 => depth 10.
    depthScene.add(plane);
    int farPlaneId = depthScene.selected;
    plane.transform.position = {0.35f, 0.65f, 1}; // Camera at z=7 => depth 6.
    depthScene.add(plane);
    int nearPlaneId = depthScene.selected;
    glDepthRange(0.2, 0.8);
    glClearDepth(0.25);
    auto planeFrame = renderer.captureDataset(depthScene, pickCamera, 321, 241);
    GLdouble restoredRange[2], restoredClear;
    glGetDoublev(GL_DEPTH_RANGE, restoredRange);
    glGetDoublev(GL_DEPTH_CLEAR_VALUE, &restoredClear);
    check(std::abs(restoredRange[0] - 0.2) < 1e-6 && std::abs(restoredRange[1] - 0.8) < 1e-6 &&
          std::abs(restoredClear - 0.25) < 1e-6, "Capture did not restore depth state.");
    glDepthRange(0, 1);
    glClearDepth(1);
    size_t farPixels = 0, nearPixels = 0;
    for (size_t p = 0; p < planeFrame.depth.size(); ++p)
    {
        int id = planeFrame.instanceIds[3*p] | (planeFrame.instanceIds[3*p+1] << 8) |
                 (planeFrame.instanceIds[3*p+2] << 16);
        if (!id) { check(planeFrame.depth[p] == 0, "Background depth must be zero."); continue; }
        check(id == farPlaneId || id == nearPlaneId, "Unexpected depth test ID.");
        float expected = id == farPlaneId ? 10.0f : 6.0f;
        check(std::abs(planeFrame.depth[p] - expected) < 0.003f,
              "Linear depth differs from known plane distance (or was flipped vertically).");
        if (id == farPlaneId) ++farPixels; else ++nearPixels;
    }
    check(farPixels > 0 && nearPixels > 0, "Known-distance planes are not both visible.");
    depthScene.objects.back().visible = false;
    auto noFront = renderer.captureDataset(depthScene, pickCamera, 321, 241);
    size_t exposed = 0;
    for (size_t p = 0; p < noFront.depth.size(); ++p)
        if (planeFrame.instanceIds[3*p] == nearPlaneId && noFront.instanceIds[3*p] == farPlaneId)
        {
            check(std::abs(noFront.depth[p] - 10.0f) < 0.003f, "Hidden near plane left stale depth.");
            ++exposed;
        }
    check(exposed > 0, "Known-distance test did not exercise occlusion.");
    depthScene.objects.front().visible = false;
    auto emptyDepth = renderer.captureDataset(depthScene, pickCamera, 37, 29);
    auto emptyPreview = makeDepthPreview(emptyDepth.depth);
    check(emptyPreview.validPixels == 0 && std::all_of(emptyDepth.depth.begin(), emptyDepth.depth.end(),
          [](float z) { return z == 0; }), "Empty scene has valid depth.");
    check(std::abs(linearCameraDepth(0, 0.05f, 500) - 0.05f) < 1e-6f &&
          std::abs(linearCameraDepth(1, 0.05f, 500) - 500) < 0.001f, "Near/far depth endpoints are wrong.");
    writeDepthNpy(folder / "depth-planes.npy", planeFrame.width, planeFrame.height, planeFrame.depth);
    std::cout << "DEPTH PASS: planes at 6/10 units, occlusion, aligned pixels, background zero, NPY export.\n";
    std::cout << "DATASET PASS: RGB/IDs aligned, 24-bit IDs, occlusion, top-left origin, clean RGB.\n";
    renderer.render(pickScene, pickCamera, 0, 0, 320, 240);
    check(renderer.pick(160, 120) == nearId, "Picking failed to choose the nearest object.");
    check(renderer.pick(0, 0) == -1, "Background should deselect.");
    pickScene.objects.back().visible = false;
    renderer.render(pickScene, pickCamera, 0, 0, 320, 240);
    check(renderer.pick(160, 120) == farId, "Hidden object intercepted a click.");
    pickScene.objects.back().visible = true;
    pickScene.objects.back().material.mode = RenderMode::Wireframe;
    renderer.render(pickScene, pickCamera, 0, 0, 400, 300);
    check(renderer.pick(200, 150) == nearId, "Picking failed after resize/in wireframe.");
    for (auto object : {makeAtom(4), makeMolecule(true), makeMolecule(false)})
    {
        pickScene.objects.clear();
        pickScene.add(std::move(object));
        renderer.render(pickScene, pickCamera, 0, 0, 320, 240);
        check(renderer.pick(160, 120) == pickScene.selected, "Compound object is not selectable.");
    }
    pickScene.objects.clear();
    pickScene.add(makeAtom(4));
    std::vector<unsigned char> before(320 * 240 * 4), after(before.size());
    renderer.animationTime = 0;
    renderer.render(pickScene, pickCamera, 0, 0, 320, 240);
    glReadPixels(0, 0, 320, 240, GL_RGBA, GL_UNSIGNED_BYTE, before.data());
    renderer.animationTime = 1;
    renderer.render(pickScene, pickCamera, 0, 0, 320, 240);
    glReadPixels(0, 0, 320, 240, GL_RGBA, GL_UNSIGNED_BYTE, after.data());
    check(before != after, "Electron animation did not change rendered pixels.");
    renderer.animationTime = 0;
    Animation clock;
    clock.advance(0.1f);
    check(clock.time == 0, "Paused animation advanced.");
    clock.playing = true;
    clock.speed = 2;
    clock.advance(0.1f);
    check(std::abs(clock.time - 0.2) < 1e-6, "Animation speed incorrect.");
    clock.playing = false;
    double paused = clock.time;
    clock.advance(0.1f);
    check(clock.time == paused, "Pause did not preserve orbit phase.");

    // Exercise real ImGui mouse events through the application's gizmo/input path.
    // Use a hidden window, so these checks never move the user's desktop cursor.
    auto savedScene = scene;
    auto savedCamera = camera;
    scene.objects.clear();
    addShape(Shape::Cube);
    addShape(Shape::Sphere);
    camera.reset();
    auto &io = ImGui::GetIO();
    bool trickle = io.ConfigInputTrickleEventQueue;
    io.ConfigInputTrickleEventQueue = false;
    auto mouseFrame = [&](ImVec2 position, bool down)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        io.AddFocusEvent(true);
        io.AddMousePosEvent(position.x, position.y);
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        gui();
        gizmo();
        input();
        ImGui::Render();
    };
    mouseFrame({720, 450}, false);
    ImVec2 center{viewportLeft + viewportWidth * 0.5f, viewportTop + viewportHeight * 0.5f};
    for (int mode = 0; mode < 2; ++mode)
    {
        gizmoMode = mode;
        scene.selection()->transform = Transform{};
        scene.selection()->transform.scale = {0.5f, 2, 1.5f};
        ImVec2 handles[3]{};
        bool found[3]{};
        for (int y = -110; y <= 110 && !(found[0] && found[1] && found[2]); y += 4)
            for (int x = -110; x <= 110; x += 4)
            {
                ImVec2 point{center.x + x, center.y + y};
                mouseFrame(point, false);
                int axis = int(ImGuizmo::GetHoveredHandleType()) -
                           int(mode == 0 ? ImGuizmo::MT_MOVE_X : ImGuizmo::MT_ROTATE_X);
                if (axis >= 0 && axis < 3 && !found[axis])
                {
                    handles[axis] = point;
                    found[axis] = true;
                }
            }
        for (int axis = 0; axis < 3; ++axis)
        {
            check(found[axis], "An X/Y/Z gizmo handle cannot be reached.");
            auto &transform = scene.selection()->transform;
            transform = Transform{};
            transform.scale = {0.5f, 2, 1.5f};
            auto start = handles[axis];
            mouseFrame(start, false);
            pendingPick = glm::vec2(-1);
            mouseFrame(start, true);
            check(gizmoActive, "Click did not start a gizmo drag.");
            ImVec2 end = mode == 0 ? ImVec2(start.x + 23, start.y + 19)
                                   : ImVec2(start.x - (start.y - center.y) * 0.35f,
                                            start.y + (start.x - center.x) * 0.35f);
            mouseFrame(end, true);
            check(pendingPick.x < 0, "Gizmo drag triggered object picking.");
            mouseFrame(end, false);
            check(!gizmoActive, "Gizmo did not release the mouse.");
            check(glm::length(transform.scale - glm::vec3(0.5f, 2, 1.5f)) < 1e-6f,
                  "Gizmo changed nonuniform scale.");
            if (mode == 0)
            {
                check(std::abs(transform.position[axis]) > 1e-4f, "Axis drag did not translate the object.");
                for (int other = 0; other < 3; ++other)
                    if (other != axis)
                        check(std::abs(transform.position[other]) < 1e-4f, "Axis drag moved another axis.");
                check(glm::length(transform.rotation) < 1e-5f, "Move gizmo changed rotation.");
            }
            else
            {
                check(glm::length(transform.rotation) > 0.01f, "Ring drag did not rotate the object.");
                check(glm::length(transform.position) < 1e-5f, "Rotation gizmo changed position.");
            }
            check(glm::length(scene.objects.front().transform.position) < 1e-5f &&
                      glm::length(scene.objects.front().transform.rotation) < 1e-5f,
                  "Gizmo edited an unselected object.");
            Camera initialCamera;
            check(glm::length(camera.position() - initialCamera.position()) < 1e-5f,
                  "Gizmo moved the camera.");
        }
    }
    io.ConfigInputTrickleEventQueue = trickle;
    scene = std::move(savedScene);
    camera = savedCamera;
    gizmoMode = 0;
    pendingPick = glm::vec2(-1);
    status = "Editor checks passed.";
    std::cout << "EDITOR PASS: scene round-trip/errors, X/Y/Z move and rotation mouse drags, focus, "
                 "picking/occlusion/resize, animation pixels.\n";
}
