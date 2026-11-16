#include "Application.h"
#include "FileDialog.h"
#include <imgui.h>
#include <algorithm>

void depthReadout(const DatasetFrame &frame, float left, float top, float width, float height)
{
    if (frame.depth.empty() || width <= 0 || height <= 0)
        return;
    auto mouse = ImGui::GetIO().MousePos;
    if (mouse.x < left || mouse.y < top || mouse.x >= left + width || mouse.y >= top + height)
        return;
    int x = std::clamp(int((mouse.x - left) / width * frame.width), 0, frame.width - 1);
    int y = std::clamp(int((mouse.y - top) / height * frame.height), 0, frame.height - 1);
    float z = frame.depth[size_t(y) * frame.width + x];
    ImGui::BeginTooltip();
    ImGui::Text("Pixel (%d, %d), top-left origin", x, y);
    if (z == 0)
        ImGui::TextUnformatted("Background / no surface (depth = 0)");
    else
        ImGui::Text("Depth Z: %.6f scene units", z);
    ImGui::TextUnformatted("Camera-forward depth, not distance to camera.");
    ImGui::EndTooltip();
}

void Application::openDataset(const std::filesystem::path &folder)
{
    datasetViewerOpen = true;
    try
    {
        auto files = datasetSamples(folder);
        datasetFiles = std::move(files);
        datasetFolder = folder;
        datasetSampleIndex = 0;
        datasetViewerDirty = true;
        datasetError.clear();
    }
    catch (const std::exception &e)
    {
        datasetError = e.what();
    }
}

void Application::datasetUi()
{
    if (!datasetViewerOpen)
        return;
    ImGui::SetNextWindowSize({850, 650}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Dataset viewer", &datasetViewerOpen))
    {
        ImGui::End();
        return;
    }
    if (ImGui::Button("Open dataset folder..."))
    {
        try
        {
            auto folder = chooseFolder(window);
            if (!folder.empty())
                openDataset(folder);
        }
        catch (const std::exception &e)
        {
            datasetError = e.what();
        }
    }
    if (!datasetFolder.empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Open in Explorer"))
        {
            try
            {
                openLocalPath(datasetFolder);
            }
            catch (const std::exception &e)
            {
                datasetError = e.what();
            }
        }
    }
    ImGui::TextWrapped("%s", datasetFolder.empty() ? "Open a sample or batch exported by this app."
                                                   : datasetFolder.u8string().c_str());
    if (!datasetFiles.empty())
    {
        ImGui::BeginDisabled(datasetSampleIndex == 0);
        if (ImGui::Button("Previous"))
        {
            --datasetSampleIndex;
            datasetViewerDirty = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(datasetSampleIndex + 1 == int(datasetFiles.size()));
        if (ImGui::Button("Next"))
        {
            ++datasetSampleIndex;
            datasetViewerDirty = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        int sample = datasetSampleIndex + 1;
        ImGui::SetNextItemWidth(250);
        if (ImGui::SliderInt("Sample", &sample, 1, int(datasetFiles.size())))
        {
            datasetSampleIndex = sample - 1;
            datasetViewerDirty = true;
        }
        const char *modes[] = {"RGB", "Depth", "Segmentation", "Bounding boxes"};
        int mode = datasetViewMode - 1;
        ImGui::SetNextItemWidth(250);
        if (ImGui::Combo("View", &mode, modes, 4))
        {
            datasetViewMode = mode + 1;
            datasetViewerDirty = true;
        }
        if (datasetViewerDirty)
        {
            datasetViewerDirty = false;
            datasetImageAvailable = false;
            datasetError.clear();
            try
            {
                datasetSample = loadSavedSample(datasetFiles.at(datasetSampleIndex));
                auto pixels = datasetSample.pixels(datasetViewMode);
                if (!pixels.empty())
                {
                    if (!datasetTexture)
                        glGenTextures(1, &datasetTexture);
                    GLint binding, alignment;
                    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
                    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
                    glBindTexture(GL_TEXTURE_2D, datasetTexture);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, datasetSample.frame.width,
                                 datasetSample.frame.height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glBindTexture(GL_TEXTURE_2D, binding);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
                    datasetImageAvailable = true;
                }
            }
            catch (const std::exception &e)
            {
                datasetError = e.what();
            }
        }
        if (datasetError.empty())
        {
            auto &f = datasetSample.frame;
            ImGui::Text("%s / %d x %d",
                        datasetFiles[datasetSampleIndex].parent_path().filename().u8string().c_str(), f.width,
                        f.height);
            ImGui::TextWrapped(
                "Saved data. Hover the image to read depth. This viewer does not change the scene.");
            if (f.depth.empty())
                ImGui::TextUnformatted("Numerical depth unavailable: depth.npy was not exported.");
            if (datasetImageAvailable)
            {
                auto space = ImGui::GetContentRegionAvail();
                float scale =
                    std::max(0.01f, std::min(space.x / f.width, std::max(1.0f, space.y - 5) / f.height));
                ImGui::Image(static_cast<ImTextureID>(datasetTexture), {f.width * scale, f.height * scale});
                auto origin = ImGui::GetItemRectMin();
                if (ImGui::IsItemHovered())
                    depthReadout(f, origin.x, origin.y, f.width * scale, f.height * scale);
            }
            else
                ImGui::TextUnformatted("This output was not saved for this sample.");
        }
    }
    if (!datasetError.empty())
        ImGui::TextWrapped("Cannot open dataset: %s", datasetError.c_str());
    ImGui::End();
}
