#pragma once
#include <filesystem>
struct GLFWwindow;
enum class FileKind
{
    Scene,
    Model,
    Texture
};
std::filesystem::path chooseFile(GLFWwindow *window, FileKind kind, bool save = false);
