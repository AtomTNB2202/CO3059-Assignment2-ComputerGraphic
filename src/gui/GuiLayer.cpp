#include "Application.h"
#include "Molecule.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace
{
bool elementCombo(const char *label, int &selected)
{
    bool changed = false;
    if (ImGui::BeginCombo(label, elements[selected].name))
    {
        for (int i = 0; i < elementCount; ++i)
            if (ImGui::Selectable(elements[i].name, selected == i))
            {
                selected = i;
                changed = true;
            }
        ImGui::EndCombo();
    }
    return changed;
}
constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
} // namespace

void Application::gui()
{
    auto size = ImGui::GetIO().DisplaySize;
    float side = std::clamp(size.x * 0.23f, 270.0f, 330.0f), top = 45, bottom = 32;
    viewportLeft = side;
    viewportTop = top;
    viewportWidth = std::max(1.0f, size.x - 2 * side);
    viewportHeight = std::max(1.0f, size.y - top - bottom);
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({size.x, top});
    ImGui::Begin("Toolbar", nullptr, panelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("CG / OBJECT LAB");
    ImGui::SameLine(210);
    if (ImGui::Button("Reset camera"))
        camera.reset();
    ImGui::SameLine();
    if (ImGui::Button("Front / 2D"))
        camera.front();
    ImGui::SameLine();
    if (ImGui::Button("Focus selected"))
        focusSelected();
    ImGui::SameLine();
    if (ImGui::Button("Save screenshot"))
        screenshotRequested = true;
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &renderer.showGrid);
    ImGui::SameLine();
    ImGui::RadioButton("Move", &gizmoMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Rotate", &gizmoMode, 1);
    ImGui::End();

    ImGui::SetNextWindowPos({0, top});
    ImGui::SetNextWindowSize({side, size.y - top - bottom});
    ImGui::Begin("Create & scene", nullptr, panelFlags);
    if (ImGui::Button("Open scene"))
        openSceneFile();
    ImGui::SameLine();
    if (ImGui::Button("Save scene..."))
        saveSceneFile();
    if (ImGui::BeginPopupModal("Replace current scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(
            "Opening a scene replaces the current objects.\nSave the current scene first if needed.");
        if (ImGui::Button("Open and replace"))
        {
            restoreSceneFile(pendingSceneFile);
            pendingSceneFile.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            pendingSceneFile.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::Separator();
    ImGui::TextDisabled("PART 1 / GEOMETRY");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##shape", &shapeChoice, shapeNames, 17);
    if (ImGui::Button("Add shape", {-1, 0}))
    {
        try
        {
            addShape(static_cast<Shape>(shapeChoice));
        }
        catch (const std::exception &error)
        {
            status = error.what();
        }
    }
    if (ImGui::CollapsingHeader("Mathematical surface"))
    {
        ImGui::TextUnformatted("z = f(x, y)");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##formula", formula.data(), formula.size());
        ImGui::TextWrapped("x, y, pi, e; + - * / ^; sin, cos, tan, sqrt, abs, exp, log");
        ImGui::SliderFloat("Domain +/-", &domain, 0.1f, 10, "%.1f");
        ImGui::SliderInt("Grid", &resolution, 4, 160);
        if (ImGui::Button("Add surface", {-1, 0}))
        {
            try
            {
                Object object;
                object.kind = ObjectKind::Surface;
                object.name = std::string("z = ") + formula.data();
                object.surfaceExpression = formula.data();
                object.surfaceExtent = domain;
                object.surfaceResolution = resolution;
                object.mesh = std::make_shared<Mesh>(makeSurface(formula.data(), domain, resolution));
                scene.addAt(std::move(object), camera.target);
                status = "Surface generated.";
            }
            catch (const std::exception &error)
            {
                status = error.what();
            }
        }
    }
    if (ImGui::CollapsingHeader("Import OBJ"))
    {
        ImGui::TextWrapped("Paste an absolute OBJ path (without quotes).");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##obj", modelPath.data(), modelPath.size());
        if (ImGui::Button("Browse OBJ..."))
            browse(false);
        if (ImGui::Button("Import model", {-1, 0}))
        {
            try
            {
                Object object;
                object.kind = ObjectKind::Imported;
                object.name = std::filesystem::u8path(modelPath.data()).filename().u8string();
                object.modelPath =
                    std::filesystem::absolute(std::filesystem::u8path(modelPath.data())).u8string();
                object.mesh = std::make_shared<Mesh>(loadObj(modelPath.data()));
                scene.addAt(std::move(object), camera.target);
                status = "OBJ imported and fitted to view.";
            }
            catch (const std::exception &error)
            {
                status = error.what();
            }
        }
        ImGui::TextWrapped("Geometry, normals and UVs are imported. Assign a texture in the inspector; MTL "
                           "materials are not loaded.");
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("PART 2 / ATOMS & MOLECULES");
    ImGui::SetNextItemWidth(-1);
    elementCombo("##element", elementChoice);
    if (ImGui::Button("Add Bohr atom", {-1, 0}))
    {
        scene.addAt(makeAtom(elementChoice), camera.target);
        status = "Added Bohr atom.";
    }
    if (ImGui::Button("Add H2O"))
        scene.addAt(makeMolecule(true), camera.target);
    ImGui::SameLine();
    if (ImGui::Button("Add CO2"))
        scene.addAt(makeMolecule(false), camera.target);
    if (ImGui::Button(animation.playing ? "Pause electrons" : "Play electrons"))
        animation.playing = !animation.playing;
    ImGui::SameLine();
    if (ImGui::Button("Reset time"))
    {
        animation.time = 0;
        animation.playing = false;
    }
    ImGui::SliderFloat("Orbit speed", &animation.speed, 0.0f, 5.0f, "%.2fx");
    if (ImGui::CollapsingHeader("Example galleries"))
    {
        ImGui::TextWrapped("Adds a gallery to the current scene.");
        if (ImGui::Button("2D shapes"))
            demo(0);
        ImGui::SameLine();
        if (ImGui::Button("3D shapes"))
            demo(1);
        if (ImGui::Button("Atoms and molecules"))
            demo(2);
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("SCENE / %d objects", static_cast<int>(scene.objects.size()));
    if (ImGui::CollapsingHeader("Batch generation"))
    {
        if (ImGui::InputInt("Samples", &batchSettings.count)) batchSettings.count = std::clamp(batchSettings.count, 1, 1000);
        if (ImGui::InputInt("Seed", &batchSettings.seed)) batchSettings.seed = std::clamp(batchSettings.seed, 0, 1000000);
        if (ImGui::InputInt("Image width", &batchSettings.width)) batchSettings.width = std::clamp(batchSettings.width, 1, 2048);
        if (ImGui::InputInt("Image height", &batchSettings.height)) batchSettings.height = std::clamp(batchSettings.height, 1, 2048);
        ImGui::Checkbox("Vary transforms", &batchSettings.transforms);
        ImGui::Checkbox("Vary colors/materials", &batchSettings.materials);
        ImGui::Checkbox("Vary textures", &batchSettings.textures);
        ImGui::Checkbox("Vary camera", &batchSettings.camera);
        ImGui::Checkbox("Vary lighting", &batchSettings.lighting);
        ImGui::Checkbox("Vary background", &batchSettings.background);
        if (ImGui::TreeNode("Variation ranges"))
        {
            ImGui::SliderFloat("Position XZ +/-", &batchSettings.positionJitter, 0, 5);
            ImGui::SliderFloat("Yaw +/- degrees", &batchSettings.yawJitter, 0, 180);
            ImGui::SliderFloat("Scale +/- fraction", &batchSettings.scaleJitter, 0, 0.5f);
            ImGui::SliderFloat("Camera yaw +/-", &batchSettings.cameraYawJitter, 0, 180);
            ImGui::SliderFloat("Camera pitch +/-", &batchSettings.cameraPitchJitter, 0, 60);
            ImGui::SliderFloat("Camera distance +/-", &batchSettings.cameraDistanceJitter, 0, 0.5f);
            ImGui::TreePop();
        }
        ImGui::TextWrapped("Each sample starts from this scene. Seed repeats the variation. The original scene is restored after export.");
        if (ImGui::Button("Generate batch", {-1, 0})) batchRequested = true;
        ImGui::TextWrapped("Progress appears in the window title and terminal. Output: dataset/batch_.../");
    }
    ImGui::BeginChild("Object list", {0, std::max(100.0f, ImGui::GetContentRegionAvail().y - 40)},
                      ImGuiChildFlags_Borders);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.24f, 0.31f, 0.48f, 1));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.29f, 0.37f, 0.55f, 1));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.33f, 0.42f, 0.62f, 1));
    for (auto &object : scene.objects)
    {
        ImGui::PushID(object.id);
        if (ImGui::Selectable(object.name.c_str(), scene.selected == object.id))
            scene.selected = object.id;
        ImGui::PopID();
    }
    ImGui::PopStyleColor(3);
    ImGui::EndChild();
    if (ImGui::Button("Delete selected"))
        scene.eraseSelected();
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
        ImGui::OpenPopup("Clear scene?");
    if (ImGui::BeginPopupModal("Clear scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Remove every object from the scene?");
        if (ImGui::Button("Clear all"))
        {
            scene.objects.clear();
            scene.selected = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Separator();
    if (ImGui::Button("Export RGB + masks + COCO + depth", {-1, 0}))
        datasetRequested = true;
    ImGui::TextWrapped("RGB, masks, COCO, depth data and preview in dataset/ next to the executable.");
    ImGui::End();

    ImGui::SetNextWindowPos({size.x - side, top});
    ImGui::SetNextWindowSize({side, size.y - top - bottom});
    ImGui::Begin("Inspector", nullptr, panelFlags);
    inspector();
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Scene lighting"))
    {
        ImGui::DragFloat3("Light", glm::value_ptr(renderer.lightPosition), 0.1f, -50, 50);
        ImGui::ColorEdit3("Background", glm::value_ptr(renderer.background));
    }
    ImGui::Spacing();
    ImGui::TextDisabled("VIEW & OBJECT CONTROLS");
    ImGui::SliderFloat("Move speed", &camera.moveSpeed, 0.5f, 20.0f, "%.1f units/s");
    ImGui::TextWrapped("W/S: forward/back\nA/D: left/right\nQ/E: down/up\nRight drag: orbit\n"
                       "Shift + right drag: pan camera\nMiddle / Shift + left drag: move selected object\n"
                       "Scroll: zoom\nF: focus selected\nR: reset camera\nDelete: remove selected");
    ImGui::TextWrapped("Click an object or its scene-list row to select. Drag the X/Y/Z arrows to move, "
                       "or choose Rotate and drag a colored ring. Orange outline marks the selection. "
                       "Hover the scene for movement keys. New objects appear at the center cross.");
    ImGui::End();

    ImGui::SetNextWindowPos({0, size.y - bottom});
    ImGui::SetNextWindowSize({size.x, bottom});
    ImGui::Begin("Status", nullptr, panelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted(status.c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", status.c_str());
    ImGui::End();
    ImGui::GetBackgroundDrawList()->AddText({viewportLeft + 16, viewportTop + 15},
                                            IM_COL32(190, 190, 190, 255), "OPENGL 3.3 / SCENE VIEW");
    if (renderer.showGrid)
        ImGui::GetBackgroundDrawList()->AddText(
            {viewportLeft + 16, viewportTop + 38}, IM_COL32(190, 190, 190, 255),
            camera.frontView ? "XY grid / X red, Y green / 1 unit" : "XZ grid / X red, Z blue / 1 unit");
    if (scene.selection())
        ImGui::GetBackgroundDrawList()->AddText(
            {viewportLeft + 16, viewportTop + 61}, IM_COL32(210, 210, 210, 255),
            gizmoMode == 0 ? "MOVE / X red, Y green, Z blue" : "ROTATE / X red, Y green, Z blue");
    ImVec2 center{viewportLeft + viewportWidth * 0.5f, viewportTop + viewportHeight * 0.5f};
    auto *overlay = ImGui::GetBackgroundDrawList();
    overlay->AddLine({center.x - 6, center.y}, {center.x + 6, center.y}, IM_COL32(215, 215, 215, 170));
    overlay->AddLine({center.x, center.y - 6}, {center.x, center.y + 6}, IM_COL32(215, 215, 215, 170));
}

void Application::inspector()
{
    auto *object = scene.selection();
    if (!object)
    {
        ImGui::TextWrapped("Select an object or add one to begin.");
        return;
    }
    ImGui::TextWrapped("%s", object->name.c_str());
    ImGui::Checkbox("Visible", &object->visible);
    ImGui::SeparatorText("Dataset label");
    ImGui::Text("Instance ID: %d", object->id);
    ImGui::Checkbox("Randomize transform in batch", &object->randomizeTransform);
    if (ImGui::InputInt("Category ID", &object->categoryId))
        object->categoryId = std::clamp(object->categoryId, 1, 1000000);
    char category[129]{};
    std::copy_n(object->categoryName.data(), std::min(object->categoryName.size(), size_t(128)), category);
    if (ImGui::InputText("Category name", category, sizeof(category)))
        object->categoryName = category;
    ImGui::TextWrapped("Same category ID/name for all objects of the same class. Save scene to keep labels.");
    ImGui::SeparatorText("Transform");
    ImGui::DragFloat3("Position", glm::value_ptr(object->transform.position), 0.05f, -100, 100, "%.2f",
                      ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat3("Rotation", glm::value_ptr(object->transform.rotation), 0.5f, -360, 360, "%.1f",
                      ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat3("Scale", glm::value_ptr(object->transform.scale), 0.02f, 0.02f, 20, "%.2f",
                      ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::Button("Reset transform"))
        object->transform = Transform{};
    ImGui::SeparatorText("Rendering");
    int mode = static_cast<int>(object->material.mode);
    if (ImGui::Combo("Mode", &mode, renderModeNames, 5))
        object->material.mode = static_cast<RenderMode>(mode);
    if (object->mesh)
        ImGui::ColorEdit3("Color", glm::value_ptr(object->material.color));
    ImGui::SliderFloat("Shininess", &object->material.shininess, 2, 128);
    if (object->material.mode == RenderMode::Texture)
    {
        ImGui::TextWrapped("PNG/JPG path (without quotes). White color gives the original image colors.");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##texture", texturePath.data(), texturePath.size());
        if (ImGui::Button("Browse image..."))
            browse(true);
        if (ImGui::Button("Load texture"))
        {
            try
            {
                object->material.texture =
                    std::make_shared<Texture>(std::filesystem::u8path(texturePath.data()));
                object->material.texturePath =
                    std::filesystem::absolute(std::filesystem::u8path(texturePath.data())).u8string();
                status = "Texture loaded.";
            }
            catch (const std::exception &error)
            {
                status = error.what();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Checker"))
        {
            object->material.texture.reset();
            object->material.texturePath.clear();
        }
    }
    if (object->mesh)
        ImGui::Text("Triangles: %zu", object->mesh->triangleCount());
    if (object->kind == ObjectKind::Atom)
    {
        ImGui::SeparatorText("Bohr model");
        if (elementCombo("Element", object->element))
            object->name = std::string(elements[object->element].name) + " (Bohr)";
        ImGui::Text("Electrons: %d", elements[object->element].electrons);
        ImGui::TextWrapped("Illustrative shell model; sizes are not to scale. Cyan spheres are electrons.");
    }
    if (object->kind == ObjectKind::Molecule)
    {
        ImGui::SeparatorText("Atoms (local positions)");
        for (size_t i = 0; i < object->atoms.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("Atom %d", static_cast<int>(i + 1));
            elementCombo("Type", object->atoms[i].element);
            ImGui::DragFloat3("XYZ", glm::value_ptr(object->atoms[i].position), 0.03f, -10, 10, "%.2f",
                              ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
        }
        ImGui::TextWrapped("Bonds follow edited atom positions. Coincident atoms have no visible bond.");
    }
}
