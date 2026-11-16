#pragma once
#include "Renderer.h"
#include "Animation.h"
#include "Batch.h"
#include "Export.h"
#include "DatasetView.h"
#include <functional>
#include <GLFW/glfw3.h>
#include <array>
#include <filesystem>
#include <string>

class Application
{
  public:
    Application(GLFWwindow *window, const std::filesystem::path &root);
    ~Application();
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
    BatchSettings batchSettings;
    ExportOptions exportOptions;
    bool exportDialogRequested = false, exportBatchMode = false, exportPending = false;
    bool exportResultRequested = false, exportSucceeded = false;
    std::array<char, 2048> exportFolder{};
    int exportWidth = 640, exportHeight = 480, previewMode = 0;
    std::filesystem::path lastExport;
    std::string exportMessage;
    GLuint exportPreviewTexture = 0;
    DatasetFrame exportPreviewFrame{};
    int exportPreviewMode = 1, exportPreviewSample = 0;
    int cachedExportSample = -1, cachedExportWidth = 0, cachedExportHeight = 0, cachedExportMode = -1;
    bool cachedExportBatch = false, exportPreviewDirty = true, exportDialogVisible = false;
    GLuint previewTexture = 0;
    DatasetFrame viewportPreviewFrame{};
    bool datasetViewerOpen = false, datasetViewerDirty = false;
    int datasetSampleIndex = 0, datasetViewMode = 1;
    GLuint datasetTexture = 0;
    std::filesystem::path datasetFolder;
    std::vector<std::filesystem::path> datasetFiles;
    SavedSample datasetSample;
    std::string datasetError;
    bool datasetImageAvailable = false;
    int previewWidth = 0, previewHeight = 0, lastPreviewMode = -1;
    double previewTime = -1;
    int dragObjectId = -1, dragMouseButton = -1;
    float viewportLeft = 310, viewportTop = 44, viewportWidth = 660, viewportHeight = 700;
    void gui();
    void inspector();
    void input();
    void addShape(Shape shape);
    void demo(int kind);
    void screenshot(const std::filesystem::path &path);
    void exportSample(const std::filesystem::path &output, int width, int height, const ExportOptions &options = {});
    void exportBatch(const std::filesystem::path &output, const BatchSettings &settings,
                     const ExportOptions &options = {}, const std::function<void(int,int)> &progress = {});
    void exportUi();
    void datasetUi();
    void openDataset(const std::filesystem::path &folder);
    void exportPreview();
    void drawPreview(int width, int height);
    void performExport();
    void exportProgress(int done, int total);
    void smokeScenes(int frame);
    void gizmo();
    void focusSelected();
    void saveSceneFile();
    void openSceneFile();
    void restoreSceneFile(const std::filesystem::path &path);
    void browse(bool texture);
};
