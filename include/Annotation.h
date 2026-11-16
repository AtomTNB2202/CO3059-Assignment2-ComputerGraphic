#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>
#include <vector>

struct InstanceLabel
{
    int instanceId, categoryId;
    std::string categoryName;
};

// Input IDs are row-major, top-left origin. Output is one self-contained COCO image.
nlohmann::json makeCoco(int width, int height, const std::vector<int> &ids,
                        const std::vector<InstanceLabel> &labels);
// Merge one sample into a dataset, remapping image/annotation IDs while keeping categories stable.
void appendCocoSample(nlohmann::json &dataset, const nlohmann::json &sample, const std::string &sampleId);

std::string yoloLabels(const nlohmann::json &coco);
void writeYolo(const std::filesystem::path &folder, const std::filesystem::path &finalFolder,
               const std::string &name, const std::filesystem::path &rgb, const nlohmann::json &coco);
