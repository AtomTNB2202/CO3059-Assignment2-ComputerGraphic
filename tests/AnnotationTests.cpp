#include "Annotation.h"
#include <functional>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}
void rejects(const std::function<void()> &call)
{
    try { call(); } catch (const std::runtime_error &) { return; }
    throw std::runtime_error("Invalid annotation input was accepted.");
}
}

int main(int argc, char **argv)
{
    try
    {
        std::vector<int> ids = {1,1,0,2,2, 1,0,0,0,2, 0,0,1,0,2, 1,1,1,2,2};
        std::vector<InstanceLabel> labels = {{1,7,"pyramid"}, {2,7,"pyramid"}, {3,11,"sphere"}};
        auto coco = makeCoco(5, 4, ids, labels);
        require(yoloLabels(coco) == "0 0.300000000 0.500000000 0.600000000 1.000000000\n0 0.800000000 0.500000000 0.400000000 1.000000000\n",
                "YOLO normalization or sparse category remapping failed.");
        require(yoloLabels(makeCoco(1,1,{3},labels)) == "1 0.500000000 0.500000000 1.000000000 1.000000000\n",
                "YOLO class indices changed when other classes became invisible.");
        require(yoloLabels(makeCoco(1,1,{0},labels)).empty(), "Empty image has YOLO labels.");
        const auto &a = coco.at("annotations");
        require(a.size() == 2 && coco.at("categories").size() == 2,
                "Invisible instance produced an annotation or category was lost.");
        require(a[0].at("bbox") == nlohmann::json({0,0,3,4}) && a[0].at("area") == 7,
                "Disconnected visible mask has wrong bbox/area.");
        require(a[1].at("bbox") == nlohmann::json({3,0,2,4}) && a[1].at("area") == 6,
                "Image-edge bbox or area is wrong.");
        // Golden strings computed independently using official pycocotools.mask.encode.
        require(a[0]["segmentation"]["counts"] == "02101O016" &&
                a[1]["segmentation"]["counts"] == "<124", "COCO compressed RLE is incompatible.");
        require(a[0]["category_id"] == a[1]["category_id"] && a[0]["id"] != a[1]["id"] &&
                a[0]["image_id"] == coco["images"][0]["id"], "Annotation references are wrong.");
        nlohmann::json merged;
        appendCocoSample(merged, coco, "sample_000001");
        appendCocoSample(merged, coco, "sample_000002");
        require(merged["images"].size() == 2 && merged["annotations"].size() == 4 &&
                merged["images"][1]["id"] == 2 && merged["annotations"][3]["id"] == 4 &&
                merged["annotations"][3]["image_id"] == 2 &&
                merged["images"][1]["file_name"] == "sample_000002/rgb.png",
                "Dataset merge did not remap IDs or image paths.");
        rejects([&] { appendCocoSample(merged, coco, "sample_000001"); });
        auto badCategories = coco;
        badCategories["categories"][0]["name"] = "different";
        rejects([&] { appendCocoSample(merged, badCategories, "sample_000003"); });
        require(merged["images"].size() == 2, "Rejected merge damaged existing dataset.");
        auto pixel = makeCoco(3,2,{0,0,0,0,0,9},{{9,2,"pixel"}})["annotations"][0];
        require(pixel["bbox"] == nlohmann::json({2,1,1,1}) && pixel["area"] == 1 &&
                pixel["segmentation"]["counts"] == "51", "One-pixel mask lost inclusive extent.");
        auto full = makeCoco(1,1,{1},{{1,1,"full"}})["annotations"][0];
        require(full["bbox"] == nlohmann::json({0,0,1,1}) &&
                full["segmentation"]["counts"] == "01", "Foreground-first RLE is wrong.");
        require(makeCoco(2,2,{0,0,0,0},labels)["annotations"].empty(), "Empty image has labels.");
        rejects([&] { makeCoco(5,4,ids,{{1,7,"pyramid"},{2,7,"sphere"}}); });
        rejects([&] { makeCoco(5,4,ids,{{1,7,"pyramid"},{2,8,"pyramid"}}); });
        rejects([&] { makeCoco(5,4,ids,{{1,0,"pyramid"},{2,7,"pyramid"}}); });
        rejects([&] { makeCoco(5,4,ids,{{1,7," "},{2,7," "}}); });
        rejects([&] { makeCoco(5,4,ids,{{1,7,"pyramid"},{1,7,"pyramid"}}); });
        rejects([&] { makeCoco(5,4,ids,{{1,7,"pyramid"}}); });
        rejects([&] { makeCoco(4,4,ids,labels); });
        if (argc == 2)
        {
            std::ofstream file(argv[1]);
            file << coco.dump(2);
            file.close();
            require(bool(file), "Cannot write annotation test fixture.");
        }
        std::cout << "PASS: visible bbox/area, clipping, empty and one-pixel masks, COCO RLE, labels and references.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
