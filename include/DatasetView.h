#pragma once
#include "Export.h"
#include <nlohmann/json.hpp>
#include <filesystem>

struct SavedSample
{
    DatasetFrame frame{};
    nlohmann::json metadata;
    std::vector<unsigned char> segmentation, boxes, depthPreview;
    std::vector<unsigned char> pixels(int mode) const;
};
std::vector<std::filesystem::path> datasetSamples(const std::filesystem::path &folder);
SavedSample loadSavedSample(const std::filesystem::path &metadataFile);
void depthReadout(const DatasetFrame &frame, float left, float top, float width, float height);
