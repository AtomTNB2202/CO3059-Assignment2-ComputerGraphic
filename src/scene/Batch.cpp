#include "Batch.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <stdexcept>

void validateBatchSettings(const BatchSettings &s)
{
    if (s.count < 1 || s.count > 1000 || s.seed < 0 || s.seed > 1000000 ||
        s.width < 1 || s.height < 1 || s.width > 2048 || s.height > 2048)
        throw std::runtime_error("Batch: count 1..1000, seed 0..1000000, dimensions 1..2048.");
    auto range = [](float x, float high) { return std::isfinite(x) && x >= 0 && x <= high; };
    if (!range(s.positionJitter, 5) || !range(s.yawJitter, 180) || !range(s.scaleJitter, 0.5f) ||
        !range(s.cameraYawJitter, 180) || !range(s.cameraPitchJitter, 60) ||
        !range(s.cameraDistanceJitter, 0.5f))
        throw std::runtime_error("Batch variation is outside supported limits.");
    if (s.texturePaths.size() > 32) throw std::runtime_error("Use at most 32 batch textures.");
    if (s.textures && s.texturePaths.empty()) throw std::runtime_error("Choose a texture pool or disable texture variation.");
}

nlohmann::json batchSettingsJson(const BatchSettings &s)
{
    return {{"count", s.count}, {"seed", s.seed}, {"width", s.width}, {"height", s.height},
        {"transforms", s.transforms}, {"materials", s.materials}, {"textures", s.textures},
        {"camera", s.camera}, {"lighting", s.lighting}, {"background", s.background},
        {"position_jitter", s.positionJitter}, {"yaw_jitter_degrees", s.yawJitter},
        {"scale_jitter_fraction", s.scaleJitter}, {"camera_yaw_jitter_degrees", s.cameraYawJitter},
        {"camera_pitch_jitter_degrees", s.cameraPitchJitter},
        {"camera_distance_jitter_fraction", s.cameraDistanceJitter}, {"texture_pool", s.texturePaths}};
}

BatchSettings loadBatchSettings(const std::filesystem::path &path)
{
    if (std::filesystem::file_size(path) > 65536) throw std::runtime_error("Batch config exceeds 64 KB.");
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open batch config.");
    nlohmann::json j; file >> j;
    BatchSettings s;
    auto defaults = batchSettingsJson(s);
    if (!j.is_object()) throw std::runtime_error("Batch config must be a JSON object.");
    for (auto it = j.begin(); it != j.end(); ++it)
        if (!defaults.contains(it.key())) throw std::runtime_error("Unknown batch setting: " + it.key());
    auto integer = [&](const char *name, int fallback)
    {
        if (!j.contains(name)) return fallback;
        if (!j[name].is_number_integer()) throw std::runtime_error(std::string(name) + " must be an integer.");
        auto value = j[name].get<int64_t>();
        if (value < 0 || value > 1000000) throw std::runtime_error("Invalid batch integer.");
        return static_cast<int>(value);
    };
    s.count = integer("count", s.count); s.seed = integer("seed", s.seed);
    s.width = integer("width", s.width); s.height = integer("height", s.height);
    s.transforms = j.value("transforms", s.transforms); s.materials = j.value("materials", s.materials);
    s.textures = j.value("textures", s.textures); s.camera = j.value("camera", s.camera);
    s.lighting = j.value("lighting", s.lighting); s.background = j.value("background", s.background);
    s.positionJitter = j.value("position_jitter", s.positionJitter);
    s.yawJitter = j.value("yaw_jitter_degrees", s.yawJitter);
    s.scaleJitter = j.value("scale_jitter_fraction", s.scaleJitter);
    s.cameraYawJitter = j.value("camera_yaw_jitter_degrees", s.cameraYawJitter);
    s.cameraPitchJitter = j.value("camera_pitch_jitter_degrees", s.cameraPitchJitter);
    s.cameraDistanceJitter = j.value("camera_distance_jitter_fraction", s.cameraDistanceJitter);
    s.texturePaths = j.value("texture_pool", s.texturePaths);
    for (auto &name : s.texturePaths)
    {
        auto asset = std::filesystem::u8path(name);
        if (asset.is_relative()) asset = std::filesystem::absolute(path).parent_path() / asset;
        if (!std::filesystem::is_regular_file(asset)) throw std::runtime_error("Missing batch texture: " + name);
        name = std::filesystem::absolute(asset).lexically_normal().u8string();
    }
    validateBatchSettings(s);
    return s;
}

SceneDocument makeBatchSample(const SceneDocument &base, const BatchSettings &s, int index)
{
    validateBatchSettings(s);
    if (index < 0 || index >= s.count) throw std::runtime_error("Invalid batch sample index.");
    std::seed_seq key{static_cast<unsigned>(s.seed), static_cast<unsigned>(index)};
    std::mt19937 random(key);
    // Define our own conversion instead of implementation-specific uniform_real_distribution.
    auto uniform = [&](float low, float high)
    { return low + (high - low) * static_cast<float>(double(random()) / 4294967296.0); };
    auto result = base;
    result.animation.playing = false; // Static dataset: freeze the current animation time.
    for (auto &object : result.scene.objects)
    {
        if (s.transforms && object.randomizeTransform)
        {
            float bottom = objectBounds(object).low.y;
            object.transform.position.x += uniform(-s.positionJitter, s.positionJitter);
            object.transform.position.z += uniform(-s.positionJitter, s.positionJitter);
            object.transform.rotation.y += uniform(-s.yawJitter, s.yawJitter);
            object.transform.scale = glm::clamp(object.transform.scale * uniform(1-s.scaleJitter, 1+s.scaleJitter),
                                                glm::vec3(0.02f), glm::vec3(20));
            object.transform.position.y += bottom - objectBounds(object).low.y;
        }
        if (s.materials)
        {
            for (int c = 0; c < 3; ++c)
                object.material.color[c] = std::clamp(object.material.color[c] * uniform(0.65f, 1.35f), 0.05f, 1.0f);
            object.material.shininess = uniform(12, 96);
        }
        if (s.textures)
        {
            size_t choice = static_cast<size_t>(random()) % (s.texturePaths.size() + 1);
            object.material.texture.reset();
            object.material.texturePath = choice ? s.texturePaths[choice - 1] : "";
            object.material.mode = choice ? RenderMode::Texture : RenderMode::Phong;
        }
    }
    if (s.camera)
    {
        result.camera.yaw += uniform(-s.cameraYawJitter, s.cameraYawJitter);
        result.camera.pitch = std::clamp(result.camera.pitch + uniform(-s.cameraPitchJitter, s.cameraPitchJitter), -85.0f, 85.0f);
        result.camera.distance = std::clamp(result.camera.distance * uniform(1-s.cameraDistanceJitter, 1+s.cameraDistanceJitter), 0.3f, 100000.0f);
        result.camera.frontView = false;
    }
    if (s.lighting)
        for (int c = 0; c < 3; ++c) result.lightPosition[c] += uniform(-2, 2);
    if (s.background)
        for (int c = 0; c < 3; ++c) result.background[c] = uniform(0.10f, 0.40f);
    return result;
}
