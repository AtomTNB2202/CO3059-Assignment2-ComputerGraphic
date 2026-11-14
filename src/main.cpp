#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "Application.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <filesystem>

namespace
{
std::filesystem::path executableDirectory(const char *argument)
{
#ifdef _WIN32
    wchar_t path[32768];
    DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    if (length == 0 || length == 32768)
        throw std::runtime_error("Cannot locate executable directory.");
    return std::filesystem::path(std::wstring(path, length)).parent_path();
#else
    return std::filesystem::absolute(argument).parent_path();
#endif
}
} // namespace

int main(int argc, char **argv)
{
    bool smokeTest = argc > 1 && std::string(argv[1]) == "--smoke-test";
    bool exportScene = argc > 1 && std::string(argv[1]) == "--export-scene";
    bool exportBatch = argc > 1 && std::string(argv[1]) == "--export-batch";
    if (exportBatch && argc != 5)
    {
        std::cerr << "Usage: --export-batch scene.json NEW_output_directory settings.json\n";
        return 1;
    }
    if (exportScene && argc != 4)
    {
        std::cerr << "Usage: --export-scene scene.json NEW_output_directory\n";
        return 1;
    }
    glfwSetErrorCallback([](int code, const char *message)
                         { std::cerr << "GLFW " << code << ": " << message << '\n'; });
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    if (smokeTest || exportScene || exportBatch)
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    GLFWwindow *window = glfwCreateWindow(
        1440, 900, "Computer Graphics Assignment 2 | Synthetic Dataset Generator", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 960, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "OpenGL loading failed.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwSwapInterval(smokeTest ? 0 : 1);
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " | " << glGetString(GL_RENDERER) << '\n';
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Inactive GUI navigation focus must not block viewport shortcuts.
    // Text fields, active controls, and modals still capture keyboard input.
    ImGui::GetIO().ConfigNavCaptureKeyboard = false;
#ifdef _WIN32
    // Use a readable system font, including Vietnamese characters, when available.
    wchar_t windowsPath[MAX_PATH];
    if (GetWindowsDirectoryW(windowsPath, MAX_PATH))
    {
        auto font = std::filesystem::path(windowsPath) / "Fonts/segoeui.ttf";
        if (std::filesystem::exists(font))
            ImGui::GetIO().Fonts->AddFontFromFileTTF(font.u8string().c_str(), 17.0f, nullptr,
                                                     ImGui::GetIO().Fonts->GetGlyphRangesVietnamese());
    }
#endif
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 0;
    style.FrameRounding = 4;
    style.GrabRounding = 4;
    style.FramePadding = {7, 5};
    style.ItemSpacing = {8, 7};
    // Blender-inspired neutral grays; blue is reserved for interaction and selection.
    auto &colors = style.Colors;
    colors[ImGuiCol_Text] = {0.88f, 0.88f, 0.88f, 1};
    colors[ImGuiCol_TextDisabled] = {0.65f, 0.65f, 0.65f, 1};
    colors[ImGuiCol_WindowBg] = {0.16f, 0.16f, 0.16f, 1};
    colors[ImGuiCol_ChildBg] = {0.14f, 0.14f, 0.14f, 1};
    colors[ImGuiCol_PopupBg] = {0.13f, 0.13f, 0.13f, 1};
    colors[ImGuiCol_Border] = {0.09f, 0.09f, 0.09f, 1};
    colors[ImGuiCol_BorderShadow] = {0, 0, 0, 0};
    colors[ImGuiCol_FrameBg] = {0.29f, 0.29f, 0.29f, 1};
    colors[ImGuiCol_FrameBgHovered] = {0.35f, 0.35f, 0.35f, 1};
    colors[ImGuiCol_FrameBgActive] = {0.24f, 0.32f, 0.46f, 1};
    colors[ImGuiCol_TitleBg] = {0.12f, 0.12f, 0.12f, 1};
    colors[ImGuiCol_TitleBgActive] = {0.20f, 0.20f, 0.20f, 1};
    colors[ImGuiCol_TitleBgCollapsed] = {0.12f, 0.12f, 0.12f, 1};
    colors[ImGuiCol_MenuBarBg] = {0.12f, 0.12f, 0.12f, 1};
    colors[ImGuiCol_ScrollbarBg] = {0.13f, 0.13f, 0.13f, 1};
    colors[ImGuiCol_ScrollbarGrab] = {0.32f, 0.32f, 0.32f, 1};
    colors[ImGuiCol_ScrollbarGrabHovered] = {0.42f, 0.42f, 0.42f, 1};
    colors[ImGuiCol_ScrollbarGrabActive] = {0.50f, 0.50f, 0.50f, 1};
    colors[ImGuiCol_CheckMark] = {0.55f, 0.70f, 0.95f, 1};
    colors[ImGuiCol_SliderGrab] = {0.38f, 0.53f, 0.76f, 1};
    colors[ImGuiCol_SliderGrabActive] = {0.52f, 0.67f, 0.91f, 1};
    colors[ImGuiCol_Button] = {0.25f, 0.25f, 0.25f, 1};
    colors[ImGuiCol_ButtonHovered] = {0.34f, 0.34f, 0.34f, 1};
    colors[ImGuiCol_ButtonActive] = {0.24f, 0.34f, 0.53f, 1};
    colors[ImGuiCol_Header] = {0.22f, 0.22f, 0.22f, 1};
    colors[ImGuiCol_HeaderHovered] = {0.30f, 0.30f, 0.30f, 1};
    colors[ImGuiCol_HeaderActive] = {0.35f, 0.35f, 0.35f, 1};
    colors[ImGuiCol_Separator] = {0.29f, 0.29f, 0.29f, 1};
    colors[ImGuiCol_SeparatorHovered] = {0.40f, 0.48f, 0.61f, 1};
    colors[ImGuiCol_SeparatorActive] = {0.46f, 0.58f, 0.78f, 1};
    colors[ImGuiCol_TextSelectedBg] = {0.24f, 0.34f, 0.53f, 0.85f};
    colors[ImGuiCol_NavCursor] = {0.55f, 0.70f, 0.95f, 1};
    colors[ImGuiCol_ModalWindowDimBg] = {0, 0, 0, 0.50f};
    bool glfwBackend = ImGui_ImplGlfw_InitForOpenGL(window, true);
    bool glBackend = glfwBackend && ImGui_ImplOpenGL3_Init("#version 330 core");
    int result = 0;
    if (!glBackend)
    {
        std::cerr << "GUI initialization failed.\n";
        result = 1;
    }
    else
    {
        try
        {
            // Destroy GPU resources before closing the OpenGL context.
            Application application(window, executableDirectory(argv[0]));
            if (exportBatch)
                application.exportBatchScene(std::filesystem::u8path(argv[2]), std::filesystem::u8path(argv[3]),
                                             std::filesystem::u8path(argv[4]));
            else if (exportScene)
                application.exportScene(std::filesystem::u8path(argv[2]), std::filesystem::u8path(argv[3]));
            else
                application.run(smokeTest);
        }
        catch (const std::exception &error)
        {
            std::cerr << "Error: " << error.what() << '\n';
            result = 1;
        }
    }
    if (glBackend)
        ImGui_ImplOpenGL3_Shutdown();
    if (glfwBackend)
        ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
