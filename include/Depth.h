#pragma once
#include <filesystem>
#include <vector>

float linearCameraDepth(float bufferDepth, float nearPlane, float farPlane);
void writeDepthNpy(const std::filesystem::path &path, int width, int height,
                    const std::vector<float> &depth);

struct DepthPreview
{
    std::vector<unsigned char> pixels;
    float minimum = 0, maximum = 0;
    size_t validPixels = 0;
};
DepthPreview makeDepthPreview(const std::vector<float> &depth);
