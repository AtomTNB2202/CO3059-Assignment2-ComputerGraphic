#include "Geometry.h"
#include "Expression.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
constexpr float pi = glm::pi<float>();
void triangle(MeshData &mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
    glm::vec3 cross = glm::cross(b - a, c - a);
    if (glm::length(cross) < 1e-7f)
        return;
    glm::vec3 normal = glm::normalize(cross);
    unsigned int first = static_cast<unsigned int>(mesh.vertices.size());
    mesh.vertices.insert(mesh.vertices.end(),
                         {{a, normal, {0, 0}}, {b, normal, {1, 0}}, {c, normal, {0.5f, 1}}});
    mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2});
}

// Ear clipping handles concave polygons (star and arrow) as well as convex ones.
MeshData polygon(const std::vector<glm::vec2> &points)
{
    MeshData mesh;
    std::vector<unsigned int> remaining;
    for (auto p : points)
    {
        remaining.push_back(static_cast<unsigned int>(mesh.vertices.size()));
        mesh.vertices.push_back({{p.x, p.y, 0}, {0, 0, 1}, (p + 1.0f) * 0.5f});
    }
    auto cross2 = [](glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; };
    while (remaining.size() > 2)
    {
        bool found = false;
        for (size_t i = 0; i < remaining.size(); ++i)
        {
            auto a = remaining[(i + remaining.size() - 1) % remaining.size()], b = remaining[i],
                 c = remaining[(i + 1) % remaining.size()];
            if (cross2(points[b] - points[a], points[c] - points[b]) <= 1e-6f)
                continue;
            bool contains = false;
            for (auto index : remaining)
            {
                if (index == a || index == b || index == c)
                    continue;
                auto p = points[index];
                if (cross2(points[b] - points[a], p - points[a]) >= -1e-6f &&
                    cross2(points[c] - points[b], p - points[b]) >= -1e-6f &&
                    cross2(points[a] - points[c], p - points[c]) >= -1e-6f)
                    contains = true;
            }
            if (contains)
                continue;
            mesh.indices.insert(mesh.indices.end(), {a, b, c});
            remaining.erase(remaining.begin() + i);
            found = true;
            break;
        }
        if (!found)
            throw std::runtime_error("Cannot triangulate polygon.");
    }
    return mesh;
}

MeshData regularPolygon(int count, bool ellipse = false, bool star = false)
{
    std::vector<glm::vec2> points;
    for (int i = 0; i < count; ++i)
    {
        float angle = pi / 2 + 2 * pi * i / count;
        float radius = star && i % 2 ? 0.43f : 1.0f;
        points.push_back({radius * std::cos(angle), radius * std::sin(angle) * (ellipse ? 0.6f : 1.0f)});
    }
    return polygon(points);
}

MeshData revolution(float bottom, float top, int segments, bool smooth)
{
    MeshData mesh;
    for (int i = 0; i < segments; ++i)
    {
        float a = 2 * pi * i / segments, b = 2 * pi * (i + 1) / segments;
        glm::vec3 p{bottom * std::cos(a), -1, bottom * std::sin(a)},
            q{bottom * std::cos(b), -1, bottom * std::sin(b)};
        glm::vec3 r{top * std::cos(a), 1, top * std::sin(a)}, s{top * std::cos(b), 1, top * std::sin(b)};
        if (smooth)
        {
            glm::vec3 na = glm::normalize(glm::vec3(std::cos(a), (bottom - top) / 2, std::sin(a)));
            glm::vec3 nb = glm::normalize(glm::vec3(std::cos(b), (bottom - top) / 2, std::sin(b)));
            unsigned int k = static_cast<unsigned int>(mesh.vertices.size());
            float u = float(i) / segments, v = float(i + 1) / segments;
            mesh.vertices.insert(mesh.vertices.end(),
                                 {{p, na, {u, 0}}, {r, na, {u, 1}}, {q, nb, {v, 0}}, {s, nb, {v, 1}}});
            mesh.indices.insert(mesh.indices.end(), {k, k + 1, k + 2});
            if (top > 0)
                mesh.indices.insert(mesh.indices.end(), {k + 2, k + 1, k + 3});
        }
        else
        {
            triangle(mesh, p, r, q);
            triangle(mesh, q, r, s);
        }
        triangle(mesh, {0, -1, 0}, p, q);
        if (top > 0)
            triangle(mesh, {0, 1, 0}, s, r);
    }
    return mesh;
}

MeshData sphere()
{
    MeshData mesh;
    constexpr int stacks = 24, slices = 48;
    for (int row = 0; row <= stacks; ++row)
    {
        float v = float(row) / stacks, theta = v * pi;
        for (int column = 0; column <= slices; ++column)
        {
            float u = float(column) / slices, phi = u * 2 * pi;
            glm::vec3 p{std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi)};
            mesh.vertices.push_back({p, p, {u, 1 - v}});
        }
    }
    for (int row = 0; row < stacks; ++row)
        for (int column = 0; column < slices; ++column)
        {
            unsigned int a = row * (slices + 1) + column, b = a + slices + 1;
            if (row != 0)
                mesh.indices.insert(mesh.indices.end(), {a, a + 1, b});
            if (row != stacks - 1)
                mesh.indices.insert(mesh.indices.end(), {a + 1, b + 1, b});
        }
    return mesh;
}

MeshData torus()
{
    MeshData mesh;
    constexpr int rings = 48, sides = 16;
    for (int i = 0; i <= rings; ++i)
        for (int j = 0; j <= sides; ++j)
        {
            float u = float(i) / rings, v = float(j) / sides, a = u * 2 * pi, b = v * 2 * pi;
            glm::vec3 normal{std::cos(a) * std::cos(b), std::sin(b), std::sin(a) * std::cos(b)};
            glm::vec3 center{0.75f * std::cos(a), 0, 0.75f * std::sin(a)};
            mesh.vertices.push_back({center + 0.25f * normal, normal, {u, v}});
        }
    for (int i = 0; i < rings; ++i)
        for (int j = 0; j < sides; ++j)
        {
            unsigned int a = i * (sides + 1) + j, b = a + sides + 1;
            mesh.indices.insert(mesh.indices.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    return mesh;
}
} // namespace

MeshData makeShape(Shape shape)
{
    switch (shape)
    {
    case Shape::Triangle:
        return regularPolygon(3);
    case Shape::Rectangle:
        return polygon({{-1, -0.7f}, {1, -0.7f}, {1, 0.7f}, {-1, 0.7f}});
    case Shape::Pentagon:
        return regularPolygon(5);
    case Shape::Hexagon:
        return regularPolygon(6);
    case Shape::Circle:
        return regularPolygon(96);
    case Shape::Ellipse:
        return regularPolygon(96, true);
    case Shape::Trapezoid:
        return polygon({{-1, -0.7f}, {1, -0.7f}, {0.6f, 0.7f}, {-0.6f, 0.7f}});
    case Shape::Star:
        return regularPolygon(10, false, true);
    case Shape::Arrow:
        return polygon(
            {{-1, -0.3f}, {0.2f, -0.3f}, {0.2f, -0.8f}, {1, 0}, {0.2f, 0.8f}, {0.2f, 0.3f}, {-1, 0.3f}});
    case Shape::Sphere:
        return sphere();
    case Shape::Cylinder:
        return revolution(0.7f, 0.7f, 48, true);
    case Shape::Cone:
        return revolution(0.8f, 0, 48, true);
    case Shape::TruncatedCone:
        return revolution(0.9f, 0.45f, 48, true);
    case Shape::Prism:
        return revolution(1, 1, 3, false);
    case Shape::Torus:
        return torus();
    case Shape::Cube:
    {
        MeshData mesh;
        // Six faces, each with independent normals and full-range UV coordinates.
        const glm::vec3 normals[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (auto n : normals)
        {
            glm::vec3 u =
                std::abs(n.y) > 0.5f ? glm::vec3(1, 0, 0) : glm::normalize(glm::cross(glm::vec3(0, 1, 0), n));
            glm::vec3 v = glm::cross(n, u);
            unsigned int k = static_cast<unsigned int>(mesh.vertices.size());
            mesh.vertices.insert(mesh.vertices.end(), {{n - u - v, n, {0, 0}},
                                                       {n + u - v, n, {1, 0}},
                                                       {n + u + v, n, {1, 1}},
                                                       {n - u + v, n, {0, 1}}});
            mesh.indices.insert(mesh.indices.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
        }
        return mesh;
    }
    case Shape::Tetrahedron:
    {
        MeshData mesh;
        glm::vec3 p[] = {{1, 1, 1}, {-1, -1, 1}, {-1, 1, -1}, {1, -1, -1}};
        int faces[][3] = {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}};
        for (auto &f : faces)
        {
            auto a = p[f[0]] * 0.65f, b = p[f[1]] * 0.65f, c = p[f[2]] * 0.65f;
            if (glm::dot(glm::cross(b - a, c - a), a) < 0)
                std::swap(b, c);
            triangle(mesh, a, b, c);
        }
        return mesh;
    }
    }
    throw std::runtime_error("Unknown shape.");
}

MeshData makeSurface(const std::string &formula, float extent, int resolution)
{
    if (!(extent >= 0.1f && extent <= 10) || resolution < 4 || resolution > 160)
        throw std::runtime_error("Surface domain/resolution outside limits.");
    Expression expression(formula);
    MeshData mesh;
    float step = 2 * extent / resolution;
    for (int row = 0; row <= resolution; ++row)
        for (int column = 0; column <= resolution; ++column)
        {
            float x = -extent + column * step, y = -extent + row * step;
            float z = static_cast<float>(expression.evaluate(x, y));
            mesh.vertices.push_back(
                {{x, y, z}, {0, 0, 0}, {float(column) / resolution, float(row) / resolution}});
        }
    for (int row = 0; row < resolution; ++row)
        for (int column = 0; column < resolution; ++column)
        {
            unsigned int a = row * (resolution + 1) + column, b = a + resolution + 1;
            mesh.indices.insert(mesh.indices.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        auto a = mesh.indices[i], b = mesh.indices[i + 1], c = mesh.indices[i + 2];
        auto normal = glm::cross(mesh.vertices[b].position - mesh.vertices[a].position,
                                 mesh.vertices[c].position - mesh.vertices[a].position);
        mesh.vertices[a].normal += normal;
        mesh.vertices[b].normal += normal;
        mesh.vertices[c].normal += normal;
    }
    for (auto &v : mesh.vertices)
        v.normal = glm::normalize(v.normal);
    return mesh;
}

void validateMesh(const MeshData &mesh)
{
    if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3)
        throw std::runtime_error("Mesh has no valid triangles.");
    for (auto i : mesh.indices)
        if (i >= mesh.vertices.size())
            throw std::runtime_error("Mesh index outside vertex buffer.");
    for (const auto &v : mesh.vertices)
    {
        for (int i = 0; i < 3; ++i)
            if (!std::isfinite(v.position[i]) || !std::isfinite(v.normal[i]))
                throw std::runtime_error("Non-finite mesh data.");
        if (!std::isfinite(v.uv.x) || !std::isfinite(v.uv.y) || glm::length(v.normal) < 0.5f)
            throw std::runtime_error("Invalid mesh normal or UV.");
    }
}
