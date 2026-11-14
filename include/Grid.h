#pragma once
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"

// A reference plane, not a selectable scene object.
class Grid
{
  public:
    explicit Grid(const std::filesystem::path &root);
    void draw(const Camera &camera, float aspect);

  private:
    Shader shader;
    Mesh plane;
};
