#pragma once
#include "Renderer.h"

struct ExportOptions
{
    bool rgb = true, depth = true, masks = true, coco = true, yolo = true;
    bool depthPreview = true, segmentationPreview = true, bboxPreview = true;
    bool any() const
    {
        return rgb || depth || masks || coco || yolo || depthPreview || segmentationPreview || bboxPreview;
    }
};

// 1 RGB, 2 depth, 3 instance colors, 4 bounding boxes. Top-left RGB bytes.
std::vector<unsigned char> previewPixels(const DatasetFrame &frame, int mode);
