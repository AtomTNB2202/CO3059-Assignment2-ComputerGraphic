#pragma once
#include "Renderer.h"
#include "Animation.h"
#include "Batch.h"
#include <GLFW/glfw3.h>
#include <array>
#include <filesystem>
#include <string>

class Application
{
  public:
    Application(GLFWwindow *window, const std::filesystem::path &root);
    void run(bool smokeTest);
    void verifyEditor();
    void exportScene(const std::filesystem::path &source, const std::filesystem::path &output);
    void exportBatchScene(const std::filesystem::path &source, const std::filesystem::path &output,
                          const std::filesystem::path &settings);

  private:
    GLFWwindow *window;
    std::filesystem::path root;
    Scene scene;
    Camera camera;
    Renderer renderer;
    Animation animation;
    int gizmoMode = 0;
    bool gizmoHovered = false, gizmoActive = false;
    glm::vec2 pendingPick{-1};
    std::filesystem::path sceneFile, pendingSceneFile;
    int shapeChoice = 9, elementChoice = 0, resolution = 48;
    float domain = 2;
    std::array<char, 257> formula{};
    std::array<char, 1024> modelPath{}, texturePath{};
    std::string status = "Ready. Add a shape, surface, atom, or molecule.";
    bool screenshotRequested = false;
    bool datasetRequested = false;
    bool batchRequested = false;
    BatchSettings batchSettings;
    int dragObjectId = -1, dragMouseButton = -1;
    float viewportLeft = 310, viewportTop = 44, viewportWidth = 660, viewportHeight = 700;
    void gui();
    void inspector();
    void input();
    void addShape(Shape shape);
    void demo(int kind);
    void screenshot(const std::filesystem::path &path);
    void exportSample(const std::filesystem::path &output, int width, int height);
    void exportBatch(const std::filesystem::path &output, const BatchSettings &settings);
    void smokeScenes(int frame);
    void gizmo();
    void focusSelected();
    void saveSceneFile();
    void openSceneFile();
    void restoreSceneFile(const std::filesystem::path &path);
    void browse(bool texture);
};
