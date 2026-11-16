#include "Application.h"
#include "Molecule.h"
#include "SceneIO.h"
#include "Annotation.h"
#include "Depth.h"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <map>

Application::Application(GLFWwindow *window, const std::filesystem::path &root)
    : window(window), root(root), renderer(root)
{
    std::strcpy(formula.data(), "sin(x)*cos(y)");
    std::string example = (root / "assets/models/pyramid.obj").u8string();
    if (example.size() < modelPath.size())
        std::strcpy(modelPath.data(), example.c_str());
    example = (root / "assets/textures/checker.png").u8string();
    if (example.size() < texturePath.size())
        std::strcpy(texturePath.data(), example.c_str());
    addShape(Shape::Cube);
    batchSettings.texturePaths = {(root / "assets/textures/checker.png").u8string(),
                                  (root / "assets/textures/checker.jpg").u8string()};
}

void Application::addShape(Shape shape)
{
    Object object;
    object.name = shapeNames[static_cast<int>(shape)];
    object.shape = shape;
    object.mesh = std::make_shared<Mesh>(makeShape(shape));
    if (static_cast<int>(shape) < 9)
        object.material.mode = RenderMode::Flat;
    scene.addAt(std::move(object), camera.target);
    status = "Added " + std::string(shapeNames[static_cast<int>(shape)]);
}

void Application::demo(int kind)
{
    // Gallery buttons preserve existing objects.
    if (kind == 0 || kind == 1)
    {
        int first = kind == 0 ? 0 : 9, count = kind == 0 ? 9 : 8;
        for (int i = 0; i < count; ++i)
        {
            addShape(static_cast<Shape>(first + i));
            auto &object = scene.objects.back();
            object.transform.position = {float(i % 3 - 1) * 3.0f, float(1 - i / 3) * 3.0f, 0};
            object.material.color = {0.2f + 0.07f * (i % 3), 0.65f, 0.85f - 0.07f * (i / 3)};
        }
        camera.reset();
        camera.front();
        camera.distance = 15;
    }
    else
    {
        auto atom = makeAtom(4);
        atom.transform.position = {-3, 0, 0};
        scene.add(std::move(atom));
        auto water = makeMolecule(true);
        scene.add(std::move(water));
        auto carbon = makeMolecule(false);
        carbon.transform.position = {3.5f, 0, 0};
        scene.add(std::move(carbon));
        camera.reset();
        camera.front();
        camera.distance = 14;
    }
}

void Application::input()
{
    if (gizmoActive)
        return;
    auto &io = ImGui::GetIO();
    bool inside = io.MousePos.x >= viewportLeft && io.MousePos.x < viewportLeft + viewportWidth &&
                  io.MousePos.y >= viewportTop && io.MousePos.y < viewportTop + viewportHeight;
    if (previewMode == 0 && inside && !io.WantCaptureMouse && !gizmoHovered && !io.KeyShift && !io.KeyCtrl &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        pendingPick = {(io.MousePos.x - viewportLeft) / viewportWidth,
                       (io.MousePos.y - viewportTop) / viewportHeight};
    // A drag belongs to the object selected when the gesture starts.
    if (dragMouseButton >= 0 && (!ImGui::IsMouseDown(dragMouseButton) || scene.selected != dragObjectId ||
                                 !glfwGetWindowAttrib(window, GLFW_FOCUSED)))
    {
        dragObjectId = -1;
        dragMouseButton = -1;
    }
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard && inside && dragMouseButton < 0)
    {
        int button = -1;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            button = ImGuiMouseButton_Middle;
        else if (io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            button = ImGuiMouseButton_Left;
        if (button >= 0)
        {
            if (auto *object = scene.selection(); object && object->visible)
            {
                auto forward = glm::normalize(camera.target - camera.position());
                if (glm::dot(object->transform.position - camera.position(), forward) > 0.05f)
                {
                    dragObjectId = object->id;
                    dragMouseButton = button;
                }
                else
                    status = "Object is behind the camera. Use Focus selected first.";
            }
            else
                status = "Select a visible object in the scene list before dragging.";
        }
    }
    if (!io.WantCaptureMouse && inside && dragMouseButton >= 0)
    {
        if (auto *object = scene.selection(); object && object->id == dragObjectId)
        {
            auto forward = glm::normalize(camera.target - camera.position());
            float depth = glm::dot(object->transform.position - camera.position(), forward);
            if (depth > 0.05f)
                scene.translateSelected(
                    camera.screenDelta(io.MouseDelta.x, io.MouseDelta.y, depth, viewportHeight));
        }
    }
    else if (!io.WantCaptureMouse && inside && dragMouseButton < 0)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            if (io.KeyShift)
                camera.pan(io.MouseDelta.x, io.MouseDelta.y, viewportHeight);
            else
                camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }
        if (io.MouseWheel != 0)
            camera.zoom(io.MouseWheel);
    }
    if (!io.WantCaptureKeyboard && dragMouseButton < 0)
    {
        // Only navigate while the mouse is over the scene, never while editing GUI fields.
        if (inside && !io.WantCaptureMouse && dragMouseButton < 0 &&
            glfwGetWindowAttrib(window, GLFW_FOCUSED))
        {
            glm::vec3 direction(0);
            if (ImGui::IsKeyDown(ImGuiKey_W))
                direction.z += 1;
            if (ImGui::IsKeyDown(ImGuiKey_S))
                direction.z -= 1;
            if (ImGui::IsKeyDown(ImGuiKey_D))
                direction.x += 1;
            if (ImGui::IsKeyDown(ImGuiKey_A))
                direction.x -= 1;
            if (ImGui::IsKeyDown(ImGuiKey_E))
                direction.y += 1;
            if (ImGui::IsKeyDown(ImGuiKey_Q))
                direction.y -= 1;
            camera.move(direction, io.DeltaTime);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            camera.reset();
        if (ImGui::IsKeyPressed(ImGuiKey_F))
            focusSelected();
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            scene.eraseSelected();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void Application::screenshot(const std::filesystem::path &path)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        throw std::runtime_error("Cannot capture a minimized window.");
    std::vector<unsigned char> pixels(size_t(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    stbi_flip_vertically_on_write(true);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot write screenshot: " + path.u8string());
    auto callback = [](void *context, void *data, int size)
    { static_cast<std::ofstream *>(context)->write(static_cast<char *>(data), size); };
    if (!stbi_write_png_to_func(callback, &file, width, height, 3, pixels.data(), width * 3) || !file)
        throw std::runtime_error("PNG write failed.");
    status = "Saved screenshots/" + path.filename().u8string() + " next to the executable.";
}

void Application::exportSample(const std::filesystem::path &output, int width, int height, const ExportOptions &options)
{
    if (!options.any()) throw std::runtime_error("Choose at least one export output.");
    if ((options.coco || options.yolo) && !options.rgb) throw std::runtime_error("COCO/YOLO require RGB images.");
    auto partial = output;
    partial += ".partial";
    if (std::filesystem::exists(output) || std::filesystem::exists(partial))
        throw std::runtime_error("Export directory already exists. Choose a new sample name.");
    auto frame = renderer.captureDataset(scene, camera, width, height);
    auto writePng = [&](const std::string &name, int channels, const unsigned char *pixels)
    {
        std::ofstream file(partial / name, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot create PNG: " + name);
        auto callback = [](void *context, void *data, int size)
        { static_cast<std::ofstream *>(context)->write(static_cast<char *>(data), size); };
        // captureDataset already flipped every array into top-left order.
        stbi_flip_vertically_on_write(false);
        int ok = stbi_write_png_to_func(callback, &file, width, height, channels, pixels, width * channels);
        file.close();
        if (!ok || !file) throw std::runtime_error("Cannot write PNG: " + name);
    };
    size_t count = size_t(width) * height;
    std::vector<int> ids(count);
    std::vector<unsigned char> preview(count * 3), mask(count);
    std::map<int, size_t> areas;
    for (size_t p = 0; p < count; ++p)
    {
        int id = frame.instanceIds[3 * p] | (frame.instanceIds[3 * p + 1] << 8) |
                 (frame.instanceIds[3 * p + 2] << 16);
        ids[p] = id;
        if (!id) continue;
        ++areas[id];
        unsigned int hash = static_cast<unsigned int>(id) * 2654435761u;
        for (int c = 0; c < 3; ++c)
            preview[3 * p + c] = static_cast<unsigned char>(64 + ((hash >> (8 * c)) & 127));
    }
    std::vector<InstanceLabel> labels;
    for (const auto &object : scene.objects)
        labels.push_back({object.id, (options.coco || options.yolo) ? object.categoryId : 1, (options.coco || options.yolo) ? object.categoryName : "object"});
    auto coco = makeCoco(width, height, ids, labels);
    auto depthPreview = makeDepthPreview(frame.depth);
    std::filesystem::create_directories(partial);
    if (options.depth) writeDepthNpy(partial / "depth.npy", width, height, frame.depth);
    if (options.depthPreview) writePng("depth_preview.png", 1, depthPreview.pixels.data());
    if (options.rgb) writePng("rgb.png", 3, frame.rgb.data());
    if (options.masks) writePng("instance_ids.png", 3, frame.instanceIds.data());
    if (options.segmentationPreview) writePng("instance_preview.png", 3, preview.data());
    auto boxed = frame.rgb;
    for (const auto &annotation : coco.at("annotations"))
    {
        const auto &box = annotation.at("bbox");
        int x0 = box[0], y0 = box[1];
        int x1 = x0 + box[2].get<int>() - 1, y1 = y0 + box[3].get<int>() - 1;
        auto dot = [&](int x, int y)
        {
            size_t p = 3 * (size_t(y) * width + x);
            boxed[p] = 255; boxed[p + 1] = 220; boxed[p + 2] = 0;
        };
        for (int x = x0; x <= x1; ++x) { dot(x, y0); dot(x, y1); }
        for (int y = y0; y <= y1; ++y) { dot(x0, y); dot(x1, y); }
    }
    if (options.bboxPreview) writePng("bbox_preview.png", 3, boxed.data());
    nlohmann::json info = {{"width", width}, {"height", height}, {"origin", "top-left"},
        {"sample_id", output.filename().u8string()}, {"scene", "scene.json"},
        {"rgb", "rgb.png"}, {"instance_ids", "instance_ids.png"},
        {"preview", "instance_preview.png"}, {"background_id", 0},
        {"annotations", "annotations.json"}, {"bbox_preview", "bbox_preview.png"},
        {"bbox_format", "[x_min,y_min,width,height] from visible pixels; top-left origin"},
        {"id_encoding", "instance_id = R + 256*G + 65536*B; channels are RGB bytes"},
        {"mask_encoding", "0 = background or other instance; 255 = visible instance"},
        {"wireframe_policy", "filled surfaces"}, {"instances", nlohmann::json::array()}};
    auto eye = camera.position();
    info["camera"] = {{"position", {eye.x, eye.y, eye.z}},
        {"target", {camera.target.x, camera.target.y, camera.target.z}},
        {"vertical_fov_degrees", camera.verticalFovDegrees()}, {"aspect", double(width) / height}};
    info["camera"]["yaw_degrees"] = camera.yaw;
    info["camera"]["pitch_degrees"] = camera.pitch;
    info["camera"]["distance"] = camera.distance;
    info["camera"]["up_world"] = {0, 1, 0};
    info["rendering"] = {{"background", {renderer.background.x, renderer.background.y, renderer.background.z}},
        {"light_position", {renderer.lightPosition.x, renderer.lightPosition.y, renderer.lightPosition.z}},
        {"animation_time", renderer.animationTime}, {"grid", false}, {"samples_per_pixel", 1}};
    info["depth"] = {{"file", "depth.npy"}, {"preview", "depth_preview.png"},
        {"dtype", "<f4"}, {"shape", {height, width}}, {"origin", "top-left"},
        {"definition", "positive camera-forward Z (negative view-space Z); not Euclidean range"},
        {"units", "scene_units"}, {"background_value", 0},
        {"near_plane", frame.nearPlane}, {"far_plane", frame.farPlane},
        {"source_buffer_bits", 24}, {"valid_pixels", depthPreview.validPixels},
        {"min_valid", depthPreview.validPixels ? nlohmann::json(depthPreview.minimum) : nlohmann::json(nullptr)},
        {"max_valid", depthPreview.validPixels ? nlohmann::json(depthPreview.maximum) : nlohmann::json(nullptr)},
        {"preview_mapping", "background=0; valid=1+round(254*(max-z)/(max-min)); constant depth=255"},
        {"preview_scale", "per-sample visible min/max; brighter is nearer"}};
    for (const auto &object : scene.objects)
    {
        std::string name = "mask_" + std::to_string(object.id) + ".png";
        size_t area = areas[object.id];
        if (area && options.masks)
        {
            for (size_t p = 0; p < count; ++p) mask[p] = ids[p] == object.id ? 255 : 0;
            writePng(name, 1, mask.data());
        }
        nlohmann::json bbox = nullptr;
        for (const auto &annotation : coco.at("annotations"))
            if (annotation.at("instance_id") == object.id) bbox = annotation.at("bbox");
        info["instances"].push_back({{"instance_id", object.id}, {"name", object.name},
            {"position", {object.transform.position.x, object.transform.position.y, object.transform.position.z}},
            {"rotation_degrees", {object.transform.rotation.x, object.transform.rotation.y, object.transform.rotation.z}},
            {"scale", {object.transform.scale.x, object.transform.scale.y, object.transform.scale.z}},
            {"randomize_transform", object.randomizeTransform}, {"model_path", object.modelPath},
            {"material", {{"color", {object.material.color.x, object.material.color.y, object.material.color.z}},
                          {"mode", static_cast<int>(object.material.mode)}, {"shininess", object.material.shininess},
                          {"texture_path", object.material.texturePath}}},
            {"category_id", object.categoryId}, {"category_name", object.categoryName}, {"bbox", bbox},
            {"visible", object.visible}, {"visible_pixels", area},
            {"mask", area && options.masks ? nlohmann::json(name) : nlohmann::json(nullptr)}});
    }
    if (options.coco)
    {
        std::ofstream annotations(partial / "annotations.json", std::ios::binary);
        annotations << coco.dump(2) << '\n';
        annotations.close();
        if (!annotations) throw std::runtime_error("Cannot write annotations.json.");
    }
    if (options.yolo) writeYolo(partial / "yolo", output / "yolo", "rgb", partial / "rgb.png", coco);
    info["yolo"] = options.yolo ? nlohmann::json("yolo/labels/rgb.txt") : nlohmann::json(nullptr);
    if (!options.rgb) info["rgb"] = nullptr;
    if (!options.masks) info["instance_ids"] = nullptr;
    if (!options.segmentationPreview) info["preview"] = nullptr;
    if (!options.bboxPreview) info["bbox_preview"] = nullptr;
    if (!options.coco) info["annotations"] = nullptr;
    if (!options.depth) info["depth"]["file"] = nullptr;
    if (!options.depthPreview) info["depth"]["preview"] = nullptr;
    std::ofstream manifest(partial / "sample.json", std::ios::binary);
    manifest << info.dump(2) << '\n';
    manifest.close();
    if (!manifest) throw std::runtime_error("Cannot write sample.json.");
    SceneDocument replay{scene, camera, animation, renderer.background, renderer.lightPosition, false};
    replay.animation.playing = false;
    replay.animation.time = renderer.animationTime;
    saveScene(partial / "scene.json", replay);
    // A final sample directory appears only once all files were written successfully.
    std::filesystem::rename(partial, output);
    status = "Exported selected outputs: " + output.u8string();
    std::cout << status << '\n';
}

void Application::exportScene(const std::filesystem::path &source, const std::filesystem::path &output)
{
    auto document = loadScene(source);
    scene = std::move(document.scene);
    camera = document.camera;
    animation = document.animation;
    renderer.background = document.background;
    renderer.lightPosition = document.lightPosition;
    renderer.animationTime = document.animation.time;
    exportSample(output, 640, 480);
}

void Application::smokeScenes(int frame)
{
    scene.objects.clear();
    scene.selected = -1;
    gizmoMode = frame == 6 ? 1 : 0;
    previewMode = frame>=7 && frame<=10 ? frame-6 : 0;
    if(frame==11) exportDialogRequested=true;
    if (frame == 0)
        demo(0);
    else if (frame == 1)
        demo(1);
    else if (frame == 2)
        demo(2);
    else if (frame == 3)
    {
        Object object;
        object.name = "z = sin(x)*cos(y)";
        object.surfaceExpression = formula.data();
        object.surfaceExtent = 2;
        object.surfaceResolution = 48;
        object.kind = ObjectKind::Surface;
        object.mesh = std::make_shared<Mesh>(makeSurface(formula.data(), 2, 48));
        scene.add(std::move(object));
        camera.reset();
        camera.pitch = 38;
        camera.yaw = 25;
    }
    else if (frame == 4)
    {
        for (int mode = 0; mode < 5; ++mode)
        {
            addShape(Shape::Sphere);
            auto &object = scene.objects.back();
            object.name = renderModeNames[mode];
            object.material.mode = static_cast<RenderMode>(mode);
            object.transform.position = {float(mode - 2) * 2.4f, 0, 0};
            object.transform.scale = {0.85f, 1.15f, 0.7f};
            object.transform.rotation = {10, 20, 5};
        }
        camera.reset();
        camera.front();
        camera.distance = 18;
    }
    else if (frame == 5)
    {
        Object object;
        object.name = "Imported pyramid";
        object.kind = ObjectKind::Imported;
        object.modelPath = (root / "assets/models/pyramid.obj").u8string();
        object.mesh = std::make_shared<Mesh>(loadObj(object.modelPath));
        object.material.texture = std::make_shared<Texture>(root / "assets/textures/checker.jpg");
        object.material.texturePath = (root / "assets/textures/checker.jpg").u8string();
        object.material.mode = RenderMode::Texture;
        object.material.color = {1, 1, 1};
        scene.add(std::move(object));
        camera.reset();
    }
    else
    {
        addShape(Shape::Cube);
        scene.selection()->transform.rotation = {20, 35, 15};
        scene.selection()->transform.scale = {1.2f, 0.7f, 1};
        camera.reset();
    }
}

void Application::run(bool smokeTest)
{
    if (smokeTest)
        verifyEditor();
    int frame = 0;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0)
        {
            glfwWaitEventsTimeout(0.1);
            continue;
        }
        if (smokeTest)
            smokeScenes(frame);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        gui();
        exportUi();
        datasetUi();
        gizmo();
        input();
        if (!smokeTest && !exportDialogVisible)
            animation.advance(ImGui::GetIO().DeltaTime);
        renderer.animationTime = animation.time;
        glViewport(0, 0, width, height);
        glClearColor(0.16f, 0.16f, 0.16f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto &io = ImGui::GetIO();
        float sx = width / io.DisplaySize.x, sy = height / io.DisplaySize.y;
        int vx = static_cast<int>(viewportLeft * sx),
            vy = static_cast<int>((io.DisplaySize.y - viewportTop - viewportHeight) * sy);
        renderer.render(scene, camera, vx, vy, std::max(1, int(viewportWidth * sx)),
                        std::max(1, int(viewportHeight * sy)));
        if (pendingPick.x >= 0)
        {
            int pickX = std::clamp(int(pendingPick.x * viewportWidth * sx), 0,
                                   std::max(0, int(viewportWidth * sx) - 1));
            int pickY = std::clamp(int((1 - pendingPick.y) * viewportHeight * sy), 0,
                                   std::max(0, int(viewportHeight * sy) - 1));
            scene.selected = renderer.pick(pickX, pickY);
            pendingPick = glm::vec2(-1);
        }
        if (previewMode != 0) drawPreview(std::max(1, int(viewportWidth * sx)), std::max(1, int(viewportHeight * sy)));
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (screenshotRequested || smokeTest)
        {
            try
            {
                auto name =
                    smokeTest
                        ? "smoke-" + std::to_string(frame) + ".png"
                        : "scene-" + std::to_string(static_cast<long long>(glfwGetTime() * 1000)) + ".png";
                screenshot(root / "screenshots" / name);
            }
            catch (const std::exception &error)
            {
                if (smokeTest)
                    throw;
                status = error.what();
            }
            screenshotRequested = false;
        }
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            throw std::runtime_error("OpenGL error: " + std::to_string(error));
        glfwSwapBuffers(window);
        if (exportPending) performExport();
        ++frame;
        if (smokeTest && frame == 13)
        {
            std::cout << "SMOKE PASS: 13 captures, 17 shapes, 5 modes, live previews, export dialog, surface, atoms, molecules, OBJ, PNG/JPG, "
                         "gizmos, "
                         "screenshots.\n";
            break;
        }
    }
}
