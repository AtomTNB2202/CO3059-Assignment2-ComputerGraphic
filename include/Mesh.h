#pragma once
#include "Geometry.h"
#include <glad/gl.h>

class Mesh
{
  public:
    explicit Mesh(const MeshData &data);
    ~Mesh();
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    void draw() const;
    const Bounds &bounds() const
    {
        return localBounds;
    }
    size_t triangleCount() const
    {
        return count / 3;
    }

  private:
    GLuint vao = 0, vertexBuffer = 0, indexBuffer = 0;
    GLsizei count = 0;
    Bounds localBounds;
};
