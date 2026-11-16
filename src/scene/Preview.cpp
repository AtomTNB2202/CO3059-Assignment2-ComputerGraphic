#include "Export.h"
#include "Depth.h"
#include <map>
#include <algorithm>
#include <stdexcept>

std::vector<unsigned char> previewPixels(const DatasetFrame &frame, int mode)
{
    if (mode < 1 || mode > 4)
        throw std::runtime_error("Unknown preview mode.");
    if (mode == 1)
        return frame.rgb;
    size_t count = size_t(frame.width) * frame.height;
    std::vector<unsigned char> result(count * 3);
    if (mode == 2)
    {
        auto depth = makeDepthPreview(frame.depth);
        for (size_t i = 0; i < count; ++i)
            for (int c = 0; c < 3; ++c)
                result[3 * i + c] = depth.pixels[i];
        return result;
    }
    struct Box
    {
        int x0, y0, x1, y1;
    };
    std::map<int, Box> boxes;
    for (int y = 0; y < frame.height; ++y)
        for (int x = 0; x < frame.width; ++x)
        {
            size_t p = size_t(y) * frame.width + x;
            int id = frame.instanceIds[3 * p] | (frame.instanceIds[3 * p + 1] << 8) |
                     (frame.instanceIds[3 * p + 2] << 16);
            if (!id)
                continue;
            unsigned hash = unsigned(id) * 2654435761u;
            for (int c = 0; c < 3; ++c)
                result[3 * p + c] = static_cast<unsigned char>(64 + ((hash >> (8 * c)) & 127));
            if (mode == 4)
            {
                auto [it, inserted] = boxes.try_emplace(id, Box{x, y, x, y});
                auto &b = it->second;
                b.x0 = std::min(b.x0, x);
                b.y0 = std::min(b.y0, y);
                b.x1 = std::max(b.x1, x);
                b.y1 = std::max(b.y1, y);
            }
        }
    if (mode == 3)
        return result;
    result = frame.rgb;
    auto dot = [&](int x, int y)
    {
        size_t p = 3 * (size_t(y) * frame.width + x);
        result[p] = 255;
        result[p + 1] = 220;
        result[p + 2] = 0;
    };
    for (const auto &[id, b] : boxes)
    {
        for (int x = b.x0; x <= b.x1; ++x)
        {
            dot(x, b.y0);
            dot(x, b.y1);
        }
        for (int y = b.y0; y <= b.y1; ++y)
        {
            dot(b.x0, y);
            dot(b.x1, y);
        }
    }
    return result;
}
