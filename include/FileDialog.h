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
std::filesystem::path chooseFolder(GLFWwindow *window);
void openLocalPath(const std::filesystem::path &path);
