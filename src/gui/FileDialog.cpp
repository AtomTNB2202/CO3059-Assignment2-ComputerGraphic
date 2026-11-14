#include "FileDialog.h"
#include <stdexcept>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

std::filesystem::path chooseFile(GLFWwindow *window, FileKind kind, bool save)
{
#ifdef _WIN32
    wchar_t filename[32768]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = glfwGetWin32Window(window);
    dialog.lpstrFile = filename;
    dialog.nMaxFile = 32768;
    dialog.lpstrFilter = kind == FileKind::Scene   ? L"Scene JSON\0*.json\0\0"
                         : kind == FileKind::Model ? L"Wavefront OBJ\0*.obj\0\0"
                                                   : L"Images (PNG/JPG)\0*.png;*.jpg;*.jpeg\0\0";
    dialog.lpstrDefExt = kind == FileKind::Scene ? L"json" : kind == FileKind::Model ? L"obj" : L"png";
    dialog.Flags =
        OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))
        return filename;
    if (DWORD error = CommDlgExtendedError())
        throw std::runtime_error("File dialog failed: " + std::to_string(error));
    return {}; // Cancel is not an error and does not modify the scene.
#else
    throw std::runtime_error("Native file dialogs currently require Windows.");
#endif
}
