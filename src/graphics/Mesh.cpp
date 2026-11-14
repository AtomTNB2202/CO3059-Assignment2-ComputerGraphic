#include "Mesh.h"
#include <cstddef>

Mesh::Mesh(const MeshData &data)
{
    validateMesh(data);
    localBounds.low = localBounds.high = data.vertices.front().position;
    for (const auto &vertex : data.vertices)
    {
        localBounds.low = glm::min(localBounds.low, vertex.position);
        localBounds.high = glm::max(localBounds.high, vertex.position);
    }
    count = static_cast<GLsizei>(data.indices.size());
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &indexBuffer);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vertex), data.vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), data.indices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void *>(offsetof(Vertex, uv)));
    glBindVertexArray(0);
}
Mesh::~Mesh()
{
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
}
void Mesh::draw() const
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}
