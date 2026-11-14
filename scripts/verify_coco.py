"""Optional export verification; Python packages are not dependencies of the C++ app.

Usage: python scripts/verify_coco.py path/to/sample_directory
Requires numpy, Pillow and official pycocotools.
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from pycocotools.coco import COCO
from pycocotools import mask as mask_utils


def verify(folder):
    folder = Path(folder)
    data = json.loads((folder / "sample.json").read_text(encoding="utf-8"))
    coco = COCO(str(folder / data["annotations"]))
    assert len(coco.imgs) == 1, "Expected one image in this sample"
    image_id, image_info = next(iter(coco.imgs.items()))
    with Image.open(folder / image_info["file_name"]) as image:
        assert image.size == (image_info["width"], image_info["height"])
    with Image.open(folder / data["instance_ids"]) as image:
        pixels = np.asarray(image.convert("RGB"), dtype=np.uint32)
    ids = pixels[:, :, 0] + 256 * pixels[:, :, 1] + 65536 * pixels[:, :, 2]
    assert ids.shape == (data["height"], data["width"])
    assert ids.shape == (image_info["height"], image_info["width"])
    assert len({cat["name"] for cat in coco.cats.values()}) == len(coco.cats)
    instances = {item["instance_id"]: item for item in data["instances"]}
    annotated = set()
    for annotation in coco.anns.values():
        instance_id = annotation["instance_id"]
        assert instance_id not in annotated
        annotated.add(instance_id)
        assert annotation["image_id"] == image_id
        assert annotation["category_id"] in coco.cats
        assert annotation["iscrowd"] == 0
        visible = ids == instance_id
        decoded = coco.annToMask(annotation).astype(bool)
        assert np.array_equal(visible, decoded), "COCO RLE differs from ID map"
        assert annotation["area"] == int(visible.sum()) > 0
        ys, xs = np.where(visible)
        expected_box = [int(xs.min()), int(ys.min()), int(xs.max() - xs.min() + 1),
                        int(ys.max() - ys.min() + 1)]
        assert annotation["bbox"] == expected_box
        assert list(mask_utils.toBbox(annotation["segmentation"])) == expected_box
        item = instances[instance_id]
        assert item["category_id"] == annotation["category_id"]
        assert item["category_name"] == coco.cats[annotation["category_id"]]["name"]
        assert item["visible_pixels"] == annotation["area"]
        assert item["bbox"] == expected_box
        with Image.open(folder / item["mask"]) as image:
            binary = np.asarray(image)
        assert set(np.unique(binary)) <= {0, 255}
        assert np.array_equal(binary == 255, visible)
    assert annotated == set(np.unique(ids)) - {0}, "Visible instance missing annotation"
    for instance_id, item in instances.items():
        if instance_id not in annotated:
            assert item["visible_pixels"] == 0 and item["mask"] is None and item["bbox"] is None
    print(f"PASS: {folder} - {len(annotated)} annotations; official COCO decode, masks, boxes and links agree")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python scripts/verify_coco.py sample_directory")
    verify(sys.argv[1])
