#pragma once
#include <glad/gl.h>
#include <filesystem>

class Texture
{
  public:
    explicit Texture(const std::filesystem::path &path);
    ~Texture();
    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;
    void bind() const;

  private:
    GLuint id = 0;
};
