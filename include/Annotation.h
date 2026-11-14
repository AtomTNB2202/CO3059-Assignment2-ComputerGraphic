#pragma once
#include <nlohmann/json.hpp>
#include <string>
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
