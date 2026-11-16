#include "FileDialog.h"
#include <stdexcept>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <shellapi.h>
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

std::filesystem::path chooseFolder(GLFWwindow *window)
{
#ifdef _WIN32
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) throw std::runtime_error("Cannot initialize folder browser.");
    struct Cleanup { bool uninit; ~Cleanup() { if(uninit) CoUninitialize(); } } cleanup{SUCCEEDED(init)};
    IFileOpenDialog *dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))
        throw std::runtime_error("Cannot create folder browser.");
    struct Release { IFileOpenDialog *p; ~Release(){p->Release();} } release{dialog};
    dialog->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
    dialog->SetTitle(L"Choose export folder");
    HRESULT result = dialog->Show(glfwGetWin32Window(window));
    if(result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return {};
    if(FAILED(result)) throw std::runtime_error("Folder browser failed.");
    IShellItem *item = nullptr;
    if(FAILED(dialog->GetResult(&item))) throw std::runtime_error("Cannot read selected folder.");
    PWSTR name = nullptr;
    result = item->GetDisplayName(SIGDN_FILESYSPATH,&name);
    item->Release();
    if(FAILED(result)) throw std::runtime_error("Cannot read folder path.");
    std::filesystem::path path(name);
    CoTaskMemFree(name);
    return path;
#else
    throw std::runtime_error("Folder browsing requires Windows; enter a path instead.");
#endif
}

void openLocalPath(const std::filesystem::path &path)
{
#ifdef _WIN32
    auto absolute = std::filesystem::absolute(path);
    if(!std::filesystem::exists(absolute)) throw std::runtime_error("Export file/folder does not exist.");
    if(reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"open",absolute.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)
        throw std::runtime_error("Could not open the exported file/folder.");
#else
    throw std::runtime_error("Open the displayed export path using your file manager.");
#endif
}
