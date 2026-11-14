#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 normal{0, 0, 1};
    glm::vec2 uv{};
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct Bounds
{
    glm::vec3 low{0}, high{0};
};

enum class Shape
{
    Triangle,
    Rectangle,
    Pentagon,
    Hexagon,
    Circle,
    Ellipse,
    Trapezoid,
    Star,
    Arrow,
    Cube,
    Sphere,
    Cylinder,
    Cone,
    TruncatedCone,
    Tetrahedron,
    Torus,
    Prism
};

inline constexpr const char *shapeNames[] = {
    "Triangle",    "Rectangle", "Pentagon", "Regular hexagon", "Circle",   "Ellipse", "Trapezoid",
    "Star",        "Arrow",     "Cube",     "Sphere",          "Cylinder", "Cone",    "Truncated cone",
    "Tetrahedron", "Torus",     "Prism"};

MeshData makeShape(Shape shape);
MeshData makeSurface(const std::string &expression, float extent, int resolution);
MeshData loadObj(const std::string &path);
void validateMesh(const MeshData &mesh);
