#include "Application.h"
#include "Annotation.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace
{
void writeJson(const std::filesystem::path &path, const nlohmann::json &value)
{
    std::ofstream file(path, std::ios::binary);
    file << value.dump(2) << '\n';
    file.close();
    if (!file) throw std::runtime_error("Cannot write dataset JSON: " + path.u8string());
}
}

void Application::exportBatch(const std::filesystem::path &output, const BatchSettings &settings,
                              const ExportOptions &options, const std::function<void(int,int)> &progress)
{
    validateBatchSettings(settings);
    if (!options.any() || ((options.coco || options.yolo) && !options.rgb)) throw std::runtime_error("Invalid export output selection.");
    auto partial = output; partial += ".partial";
    if (std::filesystem::exists(output) || std::filesystem::exists(partial))
        throw std::runtime_error("Batch directory already exists. Choose a new output directory.");
    SceneDocument base{scene, camera, animation, renderer.background, renderer.lightPosition, renderer.showGrid};
    struct Restore
    {
        Scene &scene; Camera &camera; Renderer &renderer;
        SceneDocument base;
        double time;
        GLFWwindow *window;
        std::string title;
        ~Restore()
        {
            scene = base.scene; camera = base.camera;
            renderer.background = base.background; renderer.lightPosition = base.lightPosition;
            renderer.animationTime = time; renderer.showGrid = base.showGrid;
            glfwSetWindowTitle(window, title.c_str());
        }
    } restore{scene, camera, renderer, base, renderer.animationTime, window,
               "Computer Graphics Assignment 2 | Synthetic Dataset Generator"};
    // Cache external textures once per run; object geometry remains shared on the GPU.
    std::map<std::string, std::shared_ptr<Texture>> textures;
    for (const auto &name : settings.texturePaths)
        textures.emplace(name, std::make_shared<Texture>(std::filesystem::u8path(name)));
    std::filesystem::create_directories(partial);
    saveScene(partial / "base_scene.json", base);
    nlohmann::json merged;
    nlohmann::json index = {{"format", "CO3059Dataset"}, {"version", 1},
        {"base_scene", "base_scene.json"}, {"settings_file", "settings.json"},
        {"settings", batchSettingsJson(settings)}, {"annotations", "annotations.json"},
        {"samples", nlohmann::json::array()}, {"animation_policy", "static; base scene time frozen"},
        {"reproducibility", "same source assets, build, settings and seed; GPU/driver may affect rasterization"}};
    for (int i = 0; i < settings.count; ++i)
    {
        glfwPollEvents();
        if (glfwWindowShouldClose(window)) throw std::runtime_error("Batch canceled; incomplete output has .partial suffix.");
        if (progress) progress(i, settings.count);
        auto sample = makeBatchSample(base, settings, i);
        scene = std::move(sample.scene); camera = sample.camera;
        renderer.background = sample.background; renderer.lightPosition = sample.lightPosition;
        renderer.animationTime = base.animation.time;
        for (auto &object : scene.objects)
            if (!object.material.texturePath.empty() && textures.count(object.material.texturePath))
                object.material.texture = textures.at(object.material.texturePath);
        std::ostringstream name;
        name << "sample_" << std::setw(6) << std::setfill('0') << i + 1;
        auto id = name.str();
        glfwSetWindowTitle(window, ("Generating dataset " + std::to_string(i + 1) + "/" + std::to_string(settings.count)).c_str());
        exportSample(partial / id, settings.width, settings.height, options);
        nlohmann::json metadata;
        { std::ifstream file(partial / id / "sample.json"); file >> metadata; }
        metadata["generator"] = {{"seed", settings.seed}, {"sample_index", i}, {"sample_id", id}};
        writeJson(partial / id / "sample.json", metadata);
        if (options.yolo)
        {
            // Build a flat image/label directory for the entire batch.
            std::filesystem::create_directories(partial / "yolo/images");
            std::filesystem::create_directories(partial / "yolo/labels");
            std::filesystem::copy_file(partial / id / "rgb.png", partial / "yolo/images" / (id + ".png"));
            std::filesystem::copy_file(partial / id / "yolo/labels/rgb.txt", partial / "yolo/labels" / (id + ".txt"));
            std::filesystem::copy_file(partial / id / "yolo/classes.json", partial / "yolo/classes.json", std::filesystem::copy_options::overwrite_existing);
            // Correct final paths after the outer batch .partial directory is renamed.
            std::ifstream yamlIn(partial / id / "yolo/dataset.yaml");
            std::string line, yaml;
            while (std::getline(yamlIn, line)) {
                if (line.rfind("path: ", 0) == 0)
                    line = "path: " + nlohmann::json(std::filesystem::absolute(output / "yolo").generic_u8string()).dump();
                yaml += line + "\n";
            }
            writeJson(partial / id / "sample.json", metadata);
            std::ofstream yamlOut(partial / "yolo/dataset.yaml"); yamlOut << yaml; yamlOut.close();
            if (!yamlOut) throw std::runtime_error("Cannot write YOLO dataset.yaml.");
            // Each sample also remains independently usable.
            auto from = nlohmann::json(std::filesystem::absolute(output / "yolo").generic_u8string()).dump();
            auto to = nlohmann::json(std::filesystem::absolute(output / id / "yolo").generic_u8string()).dump();
            yaml.replace(yaml.find(from), from.size(), to);
            std::ofstream localYaml(partial / id / "yolo/dataset.yaml"); localYaml << yaml; localYaml.close();
            if (!localYaml) throw std::runtime_error("Cannot write sample YOLO dataset.yaml.");
        }
        if (options.coco)
        {
            nlohmann::json annotations;
            std::ifstream file(partial / id / "annotations.json"); file >> annotations;
            appendCocoSample(merged, annotations, id);
            if (!options.depth) merged["images"].back().erase("depth_file");
        }
        index["samples"].push_back({{"sample_id", id}, {"sample_index", i}, {"image_id", i + 1},
            {"rgb", id + "/rgb.png"}, {"depth", id + "/depth.npy"},
            {"instance_ids", id + "/instance_ids.png"}, {"metadata", id + "/sample.json"},
            {"scene", id + "/scene.json"}});
        auto &entry = index["samples"].back();
        if (!options.rgb) entry["rgb"] = nullptr;
        if (!options.depth) entry["depth"] = nullptr;
        if (!options.masks) entry["instance_ids"] = nullptr;
        std::cout << "Batch " << i + 1 << '/' << settings.count << '\n';
    }
    if (options.coco) writeJson(partial / "annotations.json", merged);
    else index["annotations"] = nullptr;
    index["yolo"] = options.yolo ? nlohmann::json("yolo/dataset.yaml") : nlohmann::json(nullptr);
    writeJson(partial / "dataset.json", index);
    // settings.json uses resolved absolute texture paths and can be reused on this machine.
    writeJson(partial / "settings.json", batchSettingsJson(settings));
    std::filesystem::rename(partial, output);
    if (progress) progress(settings.count, settings.count);
    status = "Exported " + std::to_string(settings.count) + " samples: " + output.u8string();
    std::cout << status << '\n';
}

void Application::exportBatchScene(const std::filesystem::path &source, const std::filesystem::path &output,
                                  const std::filesystem::path &settings)
{
    auto document = loadScene(source);
    scene = std::move(document.scene); camera = document.camera; animation = document.animation;
    renderer.background = document.background; renderer.lightPosition = document.lightPosition;
    renderer.showGrid = document.showGrid; renderer.animationTime = document.animation.time;
    exportBatch(output, loadBatchSettings(settings));
}
