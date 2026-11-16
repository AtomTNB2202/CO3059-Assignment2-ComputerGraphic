#include "Application.h"
#include "FileDialog.h"
#include "SceneIO.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <stdexcept>

void Application::focusSelected()
{
    if (auto *object = scene.selection(); object && object->visible)
    {
        auto bounds = objectBounds(*object);
        camera.frame(bounds.low, bounds.high, viewportWidth / viewportHeight);
    }
}

void Application::gizmo()
{
    gizmoHovered = gizmoActive = false;
    if (previewMode != 0) return;
    auto *object = scene.selection();
    if (!object || !object->visible)
        return;
    if (glm::dot(object->transform.position - camera.position(),
                 glm::normalize(camera.target - camera.position())) <= 0.05f)
        return;
    auto &io = ImGui::GetIO();
    bool inside = io.MousePos.x >= viewportLeft && io.MousePos.x < viewportLeft + viewportWidth &&
                  io.MousePos.y >= viewportTop && io.MousePos.y < viewportTop + viewportHeight;
    bool allowed = dragMouseButton < 0 && !io.KeyShift && !io.KeyCtrl &&
                   !ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
                   !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) &&
                   !io.WantTextInput;
    ImGuizmo::Enable(allowed);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetGizmoSizeClipSpace(0.18f);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(viewportLeft, viewportTop, viewportWidth, viewportHeight);
    // Manipulate an unscaled rigid transform, preserving non-uniform scale exactly.
    Transform rigid = object->transform;
    rigid.scale = glm::vec3(1);
    auto matrix = rigid.matrix(), view = camera.view(),
         projection = camera.projection(viewportWidth / viewportHeight);
    auto *drawList = ImGui::GetBackgroundDrawList();
    drawList->PushClipRect({viewportLeft, viewportTop},
                           {viewportLeft + viewportWidth, viewportTop + viewportHeight}, true);
    auto operation = gizmoMode == 0 ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE;
    bool changed = ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation,
                                        ImGuizmo::WORLD, glm::value_ptr(matrix));
    drawList->PopClipRect();
    gizmoHovered = allowed && inside && ImGuizmo::IsOver(operation);
    gizmoActive = allowed && ImGuizmo::IsUsing();
    if (changed && gizmoActive)
    {
        glm::vec3 position, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), glm::value_ptr(position),
                                              glm::value_ptr(rotation), glm::value_ptr(scale));
        if (gizmoMode == 0)
            object->transform.position = position;
        else
            object->transform.rotation = rotation;
    }
}

void Application::browse(bool texture)
{
    try
    {
        auto path = chooseFile(window, texture ? FileKind::Texture : FileKind::Model);
        if (path.empty())
            return;
        auto text = path.u8string();
        auto &buffer = texture ? texturePath : modelPath;
        if (text.size() >= buffer.size())
            throw std::runtime_error("File path exceeds 1023 UTF-8 bytes.");
        std::strcpy(buffer.data(), text.c_str());
        status = texture ? "Image selected. Click Load texture." : "Model selected. Click Import model.";
    }
    catch (const std::exception &error)
    {
        status = error.what();
    }
}

void Application::saveSceneFile()
{
    try
    {
        auto path = chooseFile(window, FileKind::Scene, true);
        if (path.empty())
            return;
        SceneDocument document;
        document.scene = scene;
        document.camera = camera;
        document.animation = animation;
        document.background = renderer.background;
        document.lightPosition = renderer.lightPosition;
        document.showGrid = renderer.showGrid;
        saveScene(path, document);
        sceneFile = path;
        status = "Saved scene: " + path.filename().u8string();
    }
    catch (const std::exception &error)
    {
        status = error.what();
    }
}

void Application::openSceneFile()
{
    try
    {
        auto path = chooseFile(window, FileKind::Scene);
        if (path.empty())
            return;
        if (!scene.objects.empty())
        {
            pendingSceneFile = path;
            ImGui::OpenPopup("Replace current scene?");
        }
        else
            restoreSceneFile(path);
    }
    catch (const std::exception &error)
    {
        status = error.what();
    }
}

void Application::restoreSceneFile(const std::filesystem::path &path)
{
    try
    {
        // Load and upload everything into a temporary document before changing the live scene.
        auto document = loadScene(path);
        scene = std::move(document.scene);
        camera = document.camera;
        animation = document.animation;
        renderer.background = document.background;
        renderer.lightPosition = document.lightPosition;
        renderer.showGrid = document.showGrid;
        dragObjectId = dragMouseButton = -1;
        pendingPick = glm::vec2(-1);
        sceneFile = path;
        status = "Opened scene: " + path.filename().u8string();
    }
    catch (const std::exception &error)
    {
        status = "Scene unchanged: " + std::string(error.what());
    }
}
