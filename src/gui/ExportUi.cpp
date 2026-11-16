#include "Application.h"
#include "FileDialog.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace
{
constexpr const char *modes[] = {"Scene", "RGB", "Depth", "Segmentation", "Bounding boxes"};
}

Application::~Application()
{
    if (datasetTexture) glDeleteTextures(1, &datasetTexture);
    if (exportPreviewTexture)
        glDeleteTextures(1, &exportPreviewTexture);
    if (previewTexture)
        glDeleteTextures(1, &previewTexture);
}

void Application::drawPreview(int width, int height)
{
    // Bound readback cost; retain viewport aspect ratio when reducing resolution.
    float factor = std::min(1.0f, 1024.0f / std::max(width, height));
    width = std::max(1, int(width * factor));
    height = std::max(1, int(height * factor));
    if (lastPreviewMode != previewMode || width != previewWidth || height != previewHeight ||
        glfwGetTime() - previewTime >= 0.05)
    {
        viewportPreviewFrame = renderer.captureDataset(scene, camera, width, height);
        auto pixels = previewPixels(viewportPreviewFrame, previewMode);
        if (!previewTexture)
            glGenTextures(1, &previewTexture);
        GLint texture, alignment;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glBindTexture(GL_TEXTURE_2D, previewTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        previewWidth = width;
        previewHeight = height;
        lastPreviewMode = previewMode;
        previewTime = glfwGetTime();
    }
    ImGui::GetBackgroundDrawList()->AddImage(static_cast<ImTextureID>(previewTexture),
                                             {viewportLeft, viewportTop},
                                             {viewportLeft + viewportWidth, viewportTop + viewportHeight});
    if (previewMode == 2 && !ImGui::GetIO().WantCaptureMouse)
        depthReadout(viewportPreviewFrame, viewportLeft, viewportTop, viewportWidth, viewportHeight);
}

void Application::exportUi()
{
    exportDialogVisible = false;
    if (exportDialogRequested)
    {
        exportDialogRequested = false;
        exportPreviewDirty = true;
        exportPreviewSample = 0;
        if (!exportFolder[0])
        {
            auto folder = (root / "dataset").u8string();
            if (folder.size() < exportFolder.size())
                std::strcpy(exportFolder.data(), folder.c_str());
        }
        exportMessage.clear();
        ImGui::OpenPopup("Export dataset");
    }
    auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize({std::min(1100.0f, display.x - 40), std::min(740.0f, display.y - 40)},
                             ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints({400, 0},
                                        {std::max(400.0f, display.x - 40), std::max(200.0f, display.y - 40)});
    if (ImGui::BeginPopupModal("Export dataset", nullptr, ImGuiWindowFlags_None))
    {
        exportDialogVisible = true;
        ImGui::BeginTable("Export layout", 2, ImGuiTableFlags_Resizable);
        ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthStretch, 0.46f);
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.54f);
        ImGui::TableNextColumn();
        float bodyHeight = std::max(180.0f, ImGui::GetContentRegionAvail().y - 85);
        ImGui::BeginChild("Export settings", {0, bodyHeight});
        if (ImGui::RadioButton("Current scene", !exportBatchMode))
            exportBatchMode = false;
        ImGui::SameLine();
        if (ImGui::RadioButton("Batch", exportBatchMode))
            exportBatchMode = true;
        if (exportBatchMode)
            ImGui::TextWrapped("%d samples / seed %d / %d x %d (from Scene > Batch generation)",
                               batchSettings.count, batchSettings.seed, batchSettings.width,
                               batchSettings.height);
        else
        {
            ImGui::InputInt("Width", &exportWidth);
            ImGui::InputInt("Height", &exportHeight);
            exportWidth = std::clamp(exportWidth, 1, 2048);
            exportHeight = std::clamp(exportHeight, 1, 2048);
        }
        ImGui::SeparatorText("Destination");
        ImGui::SetNextItemWidth(std::max(120.0f, ImGui::GetContentRegionAvail().x - 90));
        ImGui::InputText("##exportFolder", exportFolder.data(), exportFolder.size());
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            try
            {
                auto folder = chooseFolder(window);
                if (!folder.empty())
                {
                    auto text = folder.u8string();
                    if (text.size() >= exportFolder.size())
                        throw std::runtime_error("Folder path is too long.");
                    std::strcpy(exportFolder.data(), text.c_str());
                }
            }
            catch (const std::exception &e)
            {
                exportMessage = e.what();
            }
        }
        ImGui::TextWrapped(
            "A new sample_... or batch_... subfolder will be created. Existing exports are kept.");
        ImGui::SeparatorText("Outputs");
        if (ImGui::Button("Assignment 2 / all outputs"))
            exportOptions = ExportOptions{};
        ImGui::BeginDisabled(exportOptions.coco || exportOptions.yolo);
        ImGui::Checkbox("RGB image", &exportOptions.rgb);
        ImGui::EndDisabled();
        ImGui::Checkbox("Depth data (.npy)", &exportOptions.depth);
        ImGui::Checkbox("Instance ID image + binary masks", &exportOptions.masks);
        if (ImGui::Checkbox("COCO labels (bounding boxes + segmentation)", &exportOptions.coco) &&
            exportOptions.coco)
            exportOptions.rgb = true;
        if (ImGui::Checkbox("YOLO detection labels (.txt)", &exportOptions.yolo) && exportOptions.yolo)
            exportOptions.rgb = true;
        ImGui::TextWrapped("COCO / YOLO include RGB. Metadata and replay scenes are always included.");
        if (ImGui::TreeNode("Extra PNG files (saved to disk)"))
        {
            ImGui::Checkbox("Save depth PNG", &exportOptions.depthPreview);
            ImGui::Checkbox("Save colored segmentation PNG", &exportOptions.segmentationPreview);
            ImGui::Checkbox("Save bounding box PNG", &exportOptions.bboxPreview);
            ImGui::TreePop();
        }
        ImGui::EndChild();
        ImGui::TableNextColumn();
        ImGui::BeginChild("Export preview", {0, bodyHeight});
        exportPreview();
        ImGui::EndChild();
        ImGui::EndTable();
        if (!exportMessage.empty())
            ImGui::TextWrapped("%s", exportMessage.c_str());
        ImGui::BeginDisabled(!exportOptions.any() || !exportFolder[0]);
        if (ImGui::Button("Export", {140, 0}))
        {
            exportPending = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (exportResultRequested)
    {
        exportResultRequested = false;
        ImGui::OpenPopup("Export result");
    }
    if (ImGui::BeginPopupModal("Export result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(exportSucceeded ? "Export complete" : "Export failed / canceled");
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 550);
        ImGui::TextUnformatted(exportMessage.c_str());
        ImGui::PopTextWrapPos();
        if (exportSucceeded)
        {
            if (ImGui::Button("View dataset")) {openDataset(lastExport); ImGui::CloseCurrentPopup();}
            ImGui::SameLine();
            if (ImGui::Button("Open folder"))
                try
                {
                    openLocalPath(lastExport);
                }
                catch (const std::exception &e)
                {
                    status = e.what();
                }
            auto sample = exportBatchMode ? lastExport / "sample_000001" : lastExport;
            std::filesystem::path image;
            for (auto name : {"rgb.png", "depth_preview.png", "instance_preview.png", "bbox_preview.png",
                              "instance_ids.png"})
                if (std::filesystem::exists(sample / name))
                {
                    image = sample / name;
                    break;
                }
            ImGui::SameLine();
            ImGui::BeginDisabled(image.empty());
            if (ImGui::Button("Open first image"))
                try
                {
                    openLocalPath(image);
                }
                catch (const std::exception &e)
                {
                    status = e.what();
                }
            ImGui::EndDisabled();
            ImGui::SameLine();
        }
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void Application::exportPreview()
{
    ImGui::SeparatorText("Image preview");
    int selected = exportPreviewMode - 1;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##exportPreviewMode", &selected, modes + 1, 4))
        exportPreviewMode = selected + 1;
    int width = exportBatchMode ? batchSettings.width : exportWidth;
    int height = exportBatchMode ? batchSettings.height : exportHeight;
    if (exportBatchMode)
    {
        int sample = exportPreviewSample + 1;
        if (ImGui::SliderInt("Sample", &sample, 1, batchSettings.count))
            exportPreviewSample = sample - 1;
        exportPreviewSample = std::clamp(exportPreviewSample, 0, batchSettings.count - 1);
        ImGui::Text("Seed %d / sample %d of %d", batchSettings.seed, exportPreviewSample + 1,
                    batchSettings.count);
    }
    ImGui::Text("%d x %d / %s", width, height, modes[exportPreviewMode]);
    if (ImGui::Button("Refresh preview"))
        exportPreviewDirty = true;
    ImGui::TextWrapped("Rendered in memory. No files are written until you click Export.");
    try
    {
        bool recapture = exportPreviewDirty || width != cachedExportWidth || height != cachedExportHeight ||
                         exportBatchMode != cachedExportBatch ||
                         (exportBatchMode && exportPreviewSample != cachedExportSample);
        if (recapture)
        {
            SceneDocument base{
                scene, camera, animation, renderer.background, renderer.lightPosition, renderer.showGrid};
            auto document =
                exportBatchMode ? makeBatchSample(base, batchSettings, exportPreviewSample) : base;
            // Batch randomization changes asset paths; resolve textures just as the batch exporter does.
            if (exportBatchMode)
                for (auto &object : document.scene.objects)
                    if (!object.material.texturePath.empty())
                        object.material.texture =
                            std::make_shared<Texture>(std::filesystem::u8path(object.material.texturePath));
            struct Restore
            {
                Renderer &r;
                glm::vec3 background, light;
                double time;
                ~Restore()
                {
                    r.background = background;
                    r.lightPosition = light;
                    r.animationTime = time;
                }
            } restore{renderer, renderer.background, renderer.lightPosition, renderer.animationTime};
            renderer.background = document.background;
            renderer.lightPosition = document.lightPosition;
            renderer.animationTime = base.animation.time;
            exportPreviewFrame = renderer.captureDataset(document.scene, document.camera, width, height);
            cachedExportWidth = width;
            cachedExportHeight = height;
            cachedExportBatch = exportBatchMode;
            cachedExportSample = exportPreviewSample;
            exportPreviewDirty = false;
        }
        if (recapture || cachedExportMode != exportPreviewMode)
        {
            auto pixels = previewPixels(exportPreviewFrame, exportPreviewMode);
            if (!exportPreviewTexture)
                glGenTextures(1, &exportPreviewTexture);
            GLint texture, alignment;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
            glBindTexture(GL_TEXTURE_2D, exportPreviewTexture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         pixels.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
            cachedExportMode = exportPreviewMode;
        }
        auto available = ImGui::GetContentRegionAvail();
        float scale = std::min(available.x / width, std::max(60.0f, available.y - 45) / height);
        ImGui::Image(static_cast<ImTextureID>(exportPreviewTexture), {width * scale, height * scale});
        auto origin = ImGui::GetItemRectMin();
        if (ImGui::IsItemHovered()) depthReadout(exportPreviewFrame, origin.x, origin.y, width*scale, height*scale);
        if (exportPreviewMode == 2)
            ImGui::TextWrapped("Near = bright; far = dark. Hover to read depth in scene units.");
    }
    catch (const std::exception &error)
    {
        exportPreviewDirty = true;
        ImGui::TextWrapped("Preview unavailable: %s", error.what());
    }
}

void Application::exportProgress(int done, int total)
{
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
        throw std::runtime_error("Export canceled; incomplete data keeps its .partial suffix.");
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    auto size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({size.x / 2, size.y / 2}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({420, 0});
    ImGui::Begin("Exporting", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Exporting %d / %d", done, total);
    ImGui::ProgressBar(float(done) / total, {-1, 0});
    bool cancel = done < total && ImGui::Button("Cancel export");
    ImGui::End();
    ImGui::Render();
    glViewport(0, 0, width, height);
    glClearColor(0.16f, 0.16f, 0.16f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    if (cancel)
        throw std::runtime_error("Export canceled; incomplete data keeps its .partial suffix.");
}

void Application::performExport()
{
    exportPending = false;
    exportSucceeded = false;
    try
    {
        auto parent = std::filesystem::u8path(exportFolder.data());
        if (parent.empty())
            throw std::runtime_error("Choose a destination folder.");
        auto id = std::chrono::system_clock::now().time_since_epoch().count();
        lastExport = std::filesystem::absolute(
            parent / ((exportBatchMode ? "batch_" : "sample_") + std::to_string(id)));
        if (exportBatchMode)
            exportBatch(lastExport, batchSettings, exportOptions,
                        [this](int d, int n) { exportProgress(d, n); });
        else
        {
            exportProgress(0, 1);
            exportSample(lastExport, exportWidth, exportHeight, exportOptions);
        }
        exportSucceeded = true;
        exportMessage = lastExport.u8string();
    }
    catch (const std::exception &e)
    {
        exportMessage = e.what();
    }
    status = exportSucceeded ? "Export complete: " + lastExport.u8string() : exportMessage;
    exportResultRequested = true;
    previewTime = -1;
}
