#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MAX_DIMENSIONS 8192
#include <stb_image.h>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

Texture::Texture(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open texture: " + path.u8string());
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    if (bytes.empty() || bytes.size() > 100 * 1024 * 1024)
        throw std::runtime_error("Texture is empty or exceeds 100 MB.");
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channels = 0;
    unsigned char *pixels =
        stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
    if (!pixels)
        throw std::runtime_error(std::string("Cannot decode texture: ") + stbi_failure_reason());
    glGenTextures(1, &id);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
Texture::~Texture()
{
    if (id)
        glDeleteTextures(1, &id);
}
void Texture::bind() const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
}
