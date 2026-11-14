#pragma once
#include "Camera.h"
#include "Scene.h"
#include "Shader.h"
#include "Grid.h"
#include <filesystem>

// All arrays use top-left origin. RGB/IDs have three bytes per pixel.
struct DatasetFrame
{
    int width, height;
    std::vector<unsigned char> rgb, instanceIds;
    std::vector<float> depth; // Positive camera-forward Z in scene units; zero is background.
    float nearPlane = 0, farPlane = 0;
};

class Renderer
{
  public:
    explicit Renderer(const std::filesystem::path &root);
    ~Renderer();
    void render(const Scene &scene, const Camera &camera, int x, int y, int width, int height);
    void draw(const Mesh &mesh, const glm::mat4 &model, const Material &material, const Camera &camera,
              float aspect);
    glm::vec3 lightPosition{4, 6, 5};
    glm::vec3 background{0.235f, 0.235f, 0.235f};
    bool showGrid = true;
    double animationTime = 0;
    int pick(int x, int y) const;
    DatasetFrame captureDataset(const Scene &scene, const Camera &camera, int width, int height);

  private:
    Shader basic, gouraud, outline;
    Mesh sphere, cylinder, ring;
    Grid grid;
    std::shared_ptr<Texture> fallback;
    GLuint pickFramebuffer = 0, pickTexture = 0, pickDepth = 0, outlineVao = 0;
    int pickWidth = 0, pickHeight = 0, pickingId = 0;
    void resizePickTarget(int width, int height);
    void renderScene(const Scene &scene, const Camera &camera, int x, int y, int width, int height,
                     bool picking);
};
