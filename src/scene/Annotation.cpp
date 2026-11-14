#include "Annotation.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>

namespace
{
std::string encodeCounts(const std::vector<int> &runs)
{
    // COCO compressed RLE: delta from two runs back (starting at index 3),
    // then signed variable-length groups of five payload bits plus continuation.
    // Format reference: cocodataset/cocoapi common/maskApi.c (rleToString).
    std::string encoded;
    for (size_t i = 0; i < runs.size(); ++i)
    {
        int64_t value = runs[i];
        if (i >= 3) value -= runs[i - 2];
        bool done;
        do
        {
            int payload = static_cast<int>(static_cast<uint64_t>(value) & 31u);
            // Floor division avoids relying on right-shift behavior for negative signed integers.
            value = value >= 0 ? value / 32 : -((-value + 31) / 32);
            done = (payload & 16) ? value == -1 : value == 0;
            encoded.push_back(static_cast<char>(48 + payload + (done ? 0 : 32)));
        } while (!done);
    }
    return encoded;
}
}

nlohmann::json makeCoco(int width, int height, const std::vector<int> &ids,
                        const std::vector<InstanceLabel> &labels)
{
    using Json = nlohmann::json;
    if (width < 1 || height < 1 || width > 4096 || height > 4096 ||
        ids.size() != size_t(width) * height)
        throw std::runtime_error("Invalid COCO dimensions or ID array length.");
    std::map<int, std::string> categories;
    std::map<std::string, int> names;
    std::set<int> instances;
    for (const auto &label : labels)
    {
        if (label.instanceId < 1 || label.instanceId > 0xFFFFFF ||
            !instances.insert(label.instanceId).second)
            throw std::runtime_error("Instance IDs must be unique positive 24-bit integers.");
        if (label.categoryId < 1 || label.categoryId > 1000000 || label.categoryName.empty() ||
            label.categoryName.size() > 128 || label.categoryName.find_first_not_of(" \t\r\n") == std::string::npos)
            throw std::runtime_error("Set a category ID (1..1000000) and non-empty category name.");
        if (categories.count(label.categoryId) && categories.at(label.categoryId) != label.categoryName)
            throw std::runtime_error("One category ID has different names. Fix labels in the inspector.");
        if (names.count(label.categoryName) && names.at(label.categoryName) != label.categoryId)
            throw std::runtime_error("One category name has different IDs. Fix labels in the inspector.");
        categories[label.categoryId] = label.categoryName;
        names[label.categoryName] = label.categoryId;
    }
    struct Bounds { int x0, y0, x1 = -1, y1 = -1, area = 0; };
    std::map<int, Bounds> bounds;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            int id = ids[size_t(y) * width + x];
            if (id == 0) continue;
            if (!instances.count(id)) throw std::runtime_error("ID map contains an unknown instance.");
            auto [it, inserted] = bounds.try_emplace(id, Bounds{width, height});
            auto &b = it->second;
            b.x0 = std::min(b.x0, x); b.y0 = std::min(b.y0, y);
            b.x1 = std::max(b.x1, x); b.y1 = std::max(b.y1, y); ++b.area;
        }
    Json coco = {{"info", {{"description", "CO3059 synthetic instance annotations"}, {"version", "1.0"}}},
        {"licenses", Json::array()}, {"images", Json::array({{{"id", 1}, {"file_name", "rgb.png"},
        {"width", width}, {"height", height}}})}, {"categories", Json::array()}, {"annotations", Json::array()}};
    for (const auto &[id, name] : categories)
        coco["categories"].push_back({{"id", id}, {"name", name}, {"supercategory", "object"}});
    for (const auto &label : labels)
    {
        if (!bounds.count(label.instanceId)) continue; // Fully occluded, hidden or out of frame.
        const auto &b = bounds.at(label.instanceId);
        std::vector<int> runs;
        bool previous = false;
        int run = 0;
        // COCO RLE scans down each column, unlike our row-major input array.
        for (int x = 0; x < width; ++x)
            for (int y = 0; y < height; ++y)
            {
                bool inside = ids[size_t(y) * width + x] == label.instanceId;
                if (inside != previous) { runs.push_back(run); run = 0; previous = inside; }
                ++run;
            }
        runs.push_back(run);
        coco["annotations"].push_back({{"id", label.instanceId}, {"instance_id", label.instanceId},
            {"image_id", 1}, {"category_id", label.categoryId}, {"iscrowd", 0}, {"area", b.area},
            {"bbox", {b.x0, b.y0, b.x1 - b.x0 + 1, b.y1 - b.y0 + 1}},
            {"segmentation", {{"size", {height, width}}, {"counts", encodeCounts(runs)}}}});
    }
    return coco;
}

void appendCocoSample(nlohmann::json &dataset, const nlohmann::json &sample, const std::string &sampleId)
{
    using Json = nlohmann::json;
    if (sampleId.empty() || sampleId == "." || sampleId == ".." || sampleId.find_first_of("/\\") != std::string::npos ||
        sample.at("images").size() != 1)
        throw std::runtime_error("Invalid sample ID or COCO sample image count.");
    if (dataset.is_null())
    {
        dataset = sample;
        dataset["images"] = Json::array();
        dataset["annotations"] = Json::array();
        dataset["info"]["description"] = "CO3059 multi-image synthetic dataset";
    }
    if (dataset.at("categories") != sample.at("categories"))
        throw std::runtime_error("Category mapping changed during batch export.");
    for (const auto &image : dataset.at("images"))
        if (image.at("sample_id") == sampleId) throw std::runtime_error("Duplicate dataset sample ID.");
    int imageId = static_cast<int>(dataset.at("images").size()) + 1;
    auto image = sample.at("images")[0];
    image["id"] = imageId;
    image["sample_id"] = sampleId;
    image["file_name"] = sampleId + "/rgb.png";
    image["depth_file"] = sampleId + "/depth.npy";
    image["metadata_file"] = sampleId + "/sample.json";
    dataset["images"].push_back(std::move(image));
    for (auto annotation : sample.at("annotations"))
    {
        annotation["id"] = dataset.at("annotations").size() + 1;
        annotation["image_id"] = imageId;
        dataset["annotations"].push_back(std::move(annotation));
    }
}
