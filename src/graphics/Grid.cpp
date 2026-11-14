#include "Grid.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace
{
MeshData gridPlane()
{
    MeshData data;
    data.vertices = {{{-1, 0, -1}, {0, 1, 0}, {}},
                     {{-1, 0, 1}, {0, 1, 0}, {}},
                     {{1, 0, 1}, {0, 1, 0}, {}},
                     {{1, 0, -1}, {0, 1, 0}, {}}};
    data.indices = {0, 1, 2, 0, 2, 3};
    return data;
}
} // namespace

Grid::Grid(const std::filesystem::path &root)
    : shader(root / "shaders/grid.vert", root / "shaders/grid.frag"), plane(gridPlane())
{
}

void Grid::draw(const Camera &camera, float aspect)
{
    // Keep a large patch around the view target. Lines remain aligned to world units.
    float extent = std::max(100.0f, camera.distance * 4);
    glm::vec3 center{std::floor(camera.target.x), -0.003f, std::floor(camera.target.z)};
    if (camera.frontView)
        center = {std::floor(camera.target.x), std::floor(camera.target.y), -0.003f};
    glm::mat4 model = glm::translate(glm::mat4(1), center);
    if (camera.frontView)
        model = glm::rotate(model, glm::radians(90.0f), {1, 0, 0});
    model = glm::scale(model, glm::vec3(extent));
    shader.use();
    shader.set("model", model);
    shader.set("view", camera.view());
    shader.set("projection", camera.projection(aspect));
    shader.set("cameraPosition", camera.position());
    shader.set("fadeDistance", std::max(40.0f, camera.distance * 5));
    shader.set("frontView", camera.frontView ? 1 : 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    plane.draw();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
