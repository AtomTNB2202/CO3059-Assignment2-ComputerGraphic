#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Animation.h"
#include <filesystem>

struct SceneDocument
{
    Scene scene;
    Camera camera;
    Animation animation;
    glm::vec3 background{0.235f}, lightPosition{4, 6, 5};
    bool showGrid = true;
};
void saveScene(const std::filesystem::path &path, const SceneDocument &document);
SceneDocument loadScene(const std::filesystem::path &path);
