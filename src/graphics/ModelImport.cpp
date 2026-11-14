#include "Geometry.h"
#include <tiny_obj_loader.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <limits>

MeshData loadObj(const std::string &path)
{
    // std::filesystem::u8path preserves Vietnamese folder/file names on Windows.
    std::ifstream file(std::filesystem::u8path(path));
    if (!file)
        throw std::runtime_error("Cannot open OBJ file: " + path);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (buffer.str().size() > 50 * 1024 * 1024)
        throw std::runtime_error("OBJ file exceeds 50 MB limit.");
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    if (!reader.ParseFromString(buffer.str(), "", config))
        throw std::runtime_error("OBJ parse failed: " + reader.Error());
    MeshData mesh;
    auto &attributes = reader.GetAttrib();
    for (const auto &shape : reader.GetShapes())
    {
        for (size_t i = 0; i + 2 < shape.mesh.indices.size(); i += 3)
        {
            Vertex vertices[3];
            bool hasNormal[3]{};
            for (int j = 0; j < 3; ++j)
            {
                auto index = shape.mesh.indices[i + j];
                if (index.vertex_index < 0 ||
                    size_t(index.vertex_index) * 3 + 2 >= attributes.vertices.size())
                    throw std::runtime_error("OBJ position index is invalid.");
                size_t p = size_t(index.vertex_index) * 3;
                vertices[j].position = {attributes.vertices[p], attributes.vertices[p + 1],
                                        attributes.vertices[p + 2]};
                if (index.normal_index >= 0 && size_t(index.normal_index) * 3 + 2 < attributes.normals.size())
                {
                    size_t n = size_t(index.normal_index) * 3;
                    glm::vec3 normal{attributes.normals[n], attributes.normals[n + 1],
                                     attributes.normals[n + 2]};
                    if (glm::length(normal) > 1e-6f)
                    {
                        vertices[j].normal = glm::normalize(normal);
                        hasNormal[j] = true;
                    }
                }
                if (index.texcoord_index >= 0 &&
                    size_t(index.texcoord_index) * 2 + 1 < attributes.texcoords.size())
                {
                    size_t uv = size_t(index.texcoord_index) * 2;
                    vertices[j].uv = {attributes.texcoords[uv], attributes.texcoords[uv + 1]};
                }
                else
                    vertices[j].uv = glm::vec2(vertices[j].position.x, vertices[j].position.z) * 0.5f + 0.5f;
            }
            glm::vec3 normal = glm::cross(vertices[1].position - vertices[0].position,
                                          vertices[2].position - vertices[0].position);
            if (glm::length(normal) < 1e-8f)
                continue;
            normal = glm::normalize(normal);
            for (int j = 0; j < 3; ++j)
            {
                if (!hasNormal[j])
                    vertices[j].normal = normal;
                mesh.indices.push_back(static_cast<unsigned int>(mesh.vertices.size()));
                mesh.vertices.push_back(vertices[j]);
            }
            if (mesh.vertices.size() > 3000000)
                throw std::runtime_error("OBJ exceeds 1 million triangles.");
        }
    }
    validateMesh(mesh);
    // Center and fit imports to a two-unit box so typical models are immediately visible.
    glm::vec3 low(std::numeric_limits<float>::max()), high(-std::numeric_limits<float>::max());
    for (const auto &v : mesh.vertices)
    {
        low = glm::min(low, v.position);
        high = glm::max(high, v.position);
    }
    float size = glm::max(high.x - low.x, glm::max(high.y - low.y, high.z - low.z));
    if (size < 1e-7f)
        throw std::runtime_error("OBJ has no spatial extent.");
    for (auto &v : mesh.vertices)
        v.position = (v.position - (low + high) * 0.5f) * (2 / size);
    return mesh;
}
