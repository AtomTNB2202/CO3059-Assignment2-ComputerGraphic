#pragma once
#include "SceneIO.h"
#include <nlohmann/json.hpp>

struct BatchSettings
{
    int count = 10, seed = 42, width = 640, height = 480;
    bool transforms = true, materials = true, textures = true;
    bool camera = true, lighting = true, background = true;
    float positionJitter = 0.2f, yawJitter = 20, scaleJitter = 0.1f;
    float cameraYawJitter = 12, cameraPitchJitter = 5, cameraDistanceJitter = 0.08f;
    std::vector<std::string> texturePaths;
};

void validateBatchSettings(const BatchSettings &settings);
nlohmann::json batchSettingsJson(const BatchSettings &settings);
BatchSettings loadBatchSettings(const std::filesystem::path &path);
// Each sample starts from base, with a deterministic RNG keyed by seed/index.
SceneDocument makeBatchSample(const SceneDocument &base, const BatchSettings &settings, int index);
