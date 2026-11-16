#include "Depth.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <regex>

std::vector<float> readDepthNpy(const std::filesystem::path &path, int width, int height)
{
    if (width < 1 || height < 1 || width > 4096 || height > 4096)
        throw std::runtime_error("Invalid depth dimensions.");
    std::ifstream file(path, std::ios::binary);
    unsigned char prefix[10]{};
    file.read(reinterpret_cast<char *>(prefix), 10);
    if (!file || std::memcmp(prefix, "\x93NUMPY", 6) || prefix[6] != 1 || prefix[7] != 0)
        throw std::runtime_error("Expected NPY 1.0 depth file.");
    size_t length = prefix[8] + (size_t(prefix[9]) << 8);
    std::string header(length, ' ');
    file.read(header.data(), static_cast<std::streamsize>(length));
    std::smatch match;
    if (!file || !std::regex_search(header, std::regex("'descr'\\s*:\\s*'<f4'")) ||
        !std::regex_search(header, std::regex("'fortran_order'\\s*:\\s*False")) ||
        !std::regex_search(header, match,
                           std::regex("'shape'\\s*:\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,?\\s*\\)")) ||
        std::stoi(match[1]) != height || std::stoi(match[2]) != width)
        throw std::runtime_error("Depth must be row-major little-endian float32 with matching dimensions.");
    std::vector<float> values(size_t(width) * height);
    for (float &v : values)
    {
        unsigned char b[4]{};
        file.read(reinterpret_cast<char *>(b), 4);
        uint32_t bits =
            uint32_t(b[0]) | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24);
        std::memcpy(&v, &bits, 4);
        if (!file || !std::isfinite(v) || v < 0)
            throw std::runtime_error("Invalid or truncated depth data.");
    }
    if (file.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("Unexpected extra depth data.");
    return values;
}

float linearCameraDepth(float bufferDepth, float nearPlane, float farPlane)
{
    // Inverse of the standard perspective projection with window depth range [0,1].
    double n = nearPlane, f = farPlane, d = bufferDepth;
    return static_cast<float>((n * f) / (f - d * (f - n)));
}

void writeDepthNpy(const std::filesystem::path &path, int width, int height, const std::vector<float> &depth)
{
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                  "Depth export requires IEEE 754 float32.");
    if (width < 1 || height < 1 || width > 4096 || height > 4096 || depth.size() != size_t(width) * height)
        throw std::runtime_error("Invalid depth dimensions.");
    // NPY 1.0: little-endian float32, row-major [height,width], 64-byte aligned header.
    std::string header = "{'descr': '<f4', 'fortran_order': False, 'shape': (" + std::to_string(height) +
                         ", " + std::to_string(width) + "), }";
    size_t padding = (64 - ((10 + header.size() + 1) % 64)) % 64;
    header.append(padding, ' ');
    header.push_back('\n');
    std::vector<unsigned char> bytes(depth.size() * 4);
    for (size_t p = 0; p < depth.size(); ++p)
    {
        if (!std::isfinite(depth[p]) || depth[p] < 0)
            throw std::runtime_error("Depth must contain finite non-negative values.");
        uint32_t bits;
        std::memcpy(&bits, &depth[p], 4);
        for (int b = 0; b < 4; ++b)
            bytes[4 * p + b] = static_cast<unsigned char>(bits >> (8 * b));
    }
    const unsigned char prefix[10] = {0x93,
                                      'N',
                                      'U',
                                      'M',
                                      'P',
                                      'Y',
                                      1,
                                      0,
                                      static_cast<unsigned char>(header.size() & 255),
                                      static_cast<unsigned char>(header.size() >> 8)};
    std::ofstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot create depth.npy.");
    file.write(reinterpret_cast<const char *>(prefix), sizeof(prefix));
    file.write(header.data(), static_cast<std::streamsize>(header.size()));
    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    file.close();
    if (!file)
        throw std::runtime_error("Cannot write depth.npy.");
}

DepthPreview makeDepthPreview(const std::vector<float> &depth)
{
    DepthPreview preview;
    preview.pixels.resize(depth.size());
    for (float z : depth)
    {
        if (!std::isfinite(z) || z < 0)
            throw std::runtime_error("Invalid depth preview value.");
        if (z == 0)
            continue;
        if (preview.validPixels++ == 0)
            preview.minimum = preview.maximum = z;
        preview.minimum = std::min(preview.minimum, z);
        preview.maximum = std::max(preview.maximum, z);
    }
    for (size_t p = 0; p < depth.size(); ++p)
        if (depth[p] > 0)
        {
            double range = double(preview.maximum) - preview.minimum;
            double normalized = range > 0 ? (preview.maximum - double(depth[p])) / range : 1;
            preview.pixels[p] =
                static_cast<unsigned char>(1 + std::lround(254 * std::clamp(normalized, 0.0, 1.0)));
        }
    return preview;
}
