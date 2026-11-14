#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <filesystem>

class Shader
{
  public:
    Shader(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath);
    ~Shader();
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    void use() const;
    void set(const char *name, int value) const;
    void set(const char *name, float value) const;
    void set(const char *name, const glm::vec3 &value) const;
    void set(const char *name, const glm::mat4 &value) const;

  private:
    GLuint program = 0;
};
