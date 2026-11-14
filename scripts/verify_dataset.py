"""Verify merged COCO plus all per-sample RGB/ID/mask/depth outputs.

Usage: python scripts/verify_dataset.py dataset_directory
Requires numpy, Pillow, pycocotools (verification only).
"""
import json
import sys
from pathlib import Path

from pycocotools.coco import COCO
from verify_coco import verify
from inspect_depth import inspect


def verify_dataset(folder):
    folder = Path(folder).resolve()
    manifest = json.loads((folder / "dataset.json").read_text(encoding="utf-8"))
    merged = COCO(str(folder / manifest["annotations"]))
    samples = manifest["samples"]
    assert len(samples) == manifest["settings"]["count"] == len(merged.imgs)
    assert (folder / manifest["base_scene"]).is_file()
    assert (folder / manifest["settings_file"]).is_file()
    ids = [item["sample_id"] for item in samples]
    assert len(set(ids)) == len(ids)
    annotation_count = 0
    for item in samples:
        sample_id = item["sample_id"]
        image_id = item["image_id"]
        sample_folder = folder / sample_id
        assert sample_folder.resolve().is_relative_to(folder)
        for key in ("rgb", "depth", "instance_ids", "metadata", "scene"):
            target = (folder / item[key]).resolve()
            assert target.is_relative_to(sample_folder.resolve()) and target.is_file()
        verify(sample_folder)
        inspect(sample_folder)
        local = json.loads((sample_folder / "annotations.json").read_text(encoding="utf-8"))
        metadata = json.loads((sample_folder / "sample.json").read_text(encoding="utf-8"))
        assert metadata["sample_id"] == sample_id
        assert metadata["generator"] == {"seed": manifest["settings"]["seed"],
                                          "sample_index": item["sample_index"], "sample_id": sample_id}
        assert local["categories"] == list(merged.cats.values())
        image = merged.imgs[image_id]
        assert image["file_name"] == item["rgb"]
        assert image["depth_file"] == item["depth"] and image["metadata_file"] == item["metadata"]
        global_annotations = merged.imgToAnns[image_id]
        local_by_instance = {a["instance_id"]: a for a in local["annotations"]}
        assert len(global_annotations) == len(local_by_instance)
        for annotation in global_annotations:
            expected = local_by_instance[annotation["instance_id"]]
            for key in ("category_id", "bbox", "area", "iscrowd", "segmentation"):
                assert annotation[key] == expected[key]
        annotation_count += len(global_annotations)
    assert annotation_count == len(merged.anns)
    print(f"DATASET PASS: {len(samples)} images, {len(merged.cats)} categories, "
          f"{annotation_count} annotations; merged links and per-pixel outputs agree")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python scripts/verify_dataset.py dataset_directory")
    verify_dataset(sys.argv[1])
