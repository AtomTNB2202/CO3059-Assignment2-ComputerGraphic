#include "DatasetView.h"
#include "Depth.h"
#include <stb_image.h>
#include <fstream>
#include <algorithm>
#include <stdexcept>

namespace
{
nlohmann::json readJson(const std::filesystem::path &path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Cannot open " + path.u8string());
    nlohmann::json value;
    in >> value;
    return value;
}
std::filesystem::path child(const std::filesystem::path &folder, const nlohmann::json &name)
{
    if (!name.is_string())
        throw std::runtime_error("Invalid dataset file reference.");
    auto rel = std::filesystem::u8path(name.get<std::string>());
    if (rel.empty() || rel.is_absolute() || rel.has_root_name())
        throw std::runtime_error("Expected relative dataset path.");
    for (const auto &part : rel)
        if (part == "..")
            throw std::runtime_error("Dataset path leaves its folder.");
    return folder / rel;
}
std::vector<unsigned char> readPng(const std::filesystem::path &path, int width, int height)
{
    auto size = std::filesystem::file_size(path);
    if (!size || size > 100 * 1024 * 1024)
        throw std::runtime_error("Invalid image size.");
    std::ifstream in(path, std::ios::binary);
    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    in.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
    if (!in)
        throw std::runtime_error("Cannot read image.");
    int w, h, channels;
    if (!stbi_info_from_memory(bytes.data(), int(bytes.size()), &w, &h, &channels) || w != width ||
        h != height)
        throw std::runtime_error("Image dimensions disagree with sample metadata.");
    stbi_set_flip_vertically_on_load(false);
    auto *data = stbi_load_from_memory(bytes.data(), int(bytes.size()), &w, &h, &channels, 3);
    stbi_set_flip_vertically_on_load(true);
    if (!data)
        throw std::runtime_error("Cannot decode dataset image.");
    std::vector<unsigned char> result(data, data + size_t(w) * h * 3);
    stbi_image_free(data);
    return result;
}
} // namespace

std::vector<std::filesystem::path> datasetSamples(const std::filesystem::path &folder)
{
    std::vector<std::filesystem::path> paths;
    if (std::filesystem::exists(folder / "dataset.json"))
    {
        auto index = readJson(folder / "dataset.json");
        for (const auto &s : index.at("samples"))
            paths.push_back(child(folder, s.at("metadata")));
    }
    else if (std::filesystem::exists(folder / "sample.json"))
        paths.push_back(folder / "sample.json");
    else
        throw std::runtime_error("Choose an exported batch (dataset.json) or sample (sample.json) folder.");
    if (paths.empty())
        throw std::runtime_error("This dataset contains no samples.");
    return paths;
}

SavedSample loadSavedSample(const std::filesystem::path &metadataFile)
{
    SavedSample s;
    s.metadata = readJson(metadataFile);
    s.frame.width = s.metadata.at("width");
    s.frame.height = s.metadata.at("height");
    if (s.frame.width < 1 || s.frame.height < 1 || s.frame.width > 4096 || s.frame.height > 4096)
        throw std::runtime_error("Unsupported sample dimensions.");
    auto folder = metadataFile.parent_path();
    auto png = [&](const nlohmann::json &name)
    {
        return name.is_null() ? std::vector<unsigned char>{}
                              : readPng(child(folder, name), s.frame.width, s.frame.height);
    };
    s.frame.rgb = png(s.metadata.value("rgb", nlohmann::json(nullptr)));
    s.frame.instanceIds = png(s.metadata.value("instance_ids", nlohmann::json(nullptr)));
    s.segmentation = png(s.metadata.value("preview", nlohmann::json(nullptr)));
    s.boxes = png(s.metadata.value("bbox_preview", nlohmann::json(nullptr)));
    if (s.metadata.contains("depth"))
    {
        auto d = s.metadata.at("depth");
        if (d.contains("file") && !d["file"].is_null())
            s.frame.depth = readDepthNpy(child(folder, d["file"]), s.frame.width, s.frame.height);
        s.depthPreview = png(d.value("preview", nlohmann::json(nullptr)));
    }
    // Bounding boxes remain viewable when the optional PNG and masks were not exported.
    if (s.boxes.empty() && !s.frame.rgb.empty())
    {
        s.boxes = s.frame.rgb;
        for (const auto &o : s.metadata.at("instances"))
        {
            if (!o.contains("bbox") || o["bbox"].is_null())
                continue;
            const auto &b = o["bbox"];
            int x = b.at(0), y = b.at(1), w = b.at(2), h = b.at(3);
            if (x < 0 || y < 0 || w < 1 || h < 1 || w > s.frame.width || h > s.frame.height ||
                x > s.frame.width - w || y > s.frame.height - h)
                throw std::runtime_error("Invalid saved bounding box.");
            auto dot = [&](int xx, int yy)
            {
                size_t p = (size_t(yy) * s.frame.width + xx) * 3;
                s.boxes[p] = 255;
                s.boxes[p + 1] = 220;
                s.boxes[p + 2] = 0;
            };
            for (int xx = x; xx < x + w; ++xx)
            {
                dot(xx, y);
                dot(xx, y + h - 1);
            }
            for (int yy = y; yy < y + h; ++yy)
            {
                dot(x, yy);
                dot(x + w - 1, yy);
            }
        }
    }
    return s;
}

std::vector<unsigned char> SavedSample::pixels(int mode) const
{
    if (mode == 1)
        return frame.rgb;
    if (mode == 2)
        return frame.depth.empty() ? depthPreview : previewPixels(frame, 2);
    if (mode == 3)
        return frame.instanceIds.empty() ? segmentation : previewPixels(frame, 3);
    if (mode == 4)
        return boxes;
    return {};
}
