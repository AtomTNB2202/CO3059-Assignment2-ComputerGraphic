"""Verify a saved depth map and print numeric values. Requires numpy and Pillow.

python scripts/inspect_depth.py sample_directory [--pixel X Y]
"""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image


def inspect(folder, pixel=None):
    folder = Path(folder)
    sample = json.loads((folder / "sample.json").read_text(encoding="utf-8"))
    info = sample["depth"]
    depth = np.load(folder / info["file"], allow_pickle=False)
    assert depth.shape == (sample["height"], sample["width"]) == tuple(info["shape"])
    assert depth.dtype == np.dtype("<f4") and depth.flags.c_contiguous
    assert np.isfinite(depth).all() and (depth >= 0).all()
    with Image.open(folder / sample["rgb"]) as image:
        assert image.size == (depth.shape[1], depth.shape[0])
    with Image.open(folder / sample["instance_ids"]) as image:
        rgb = np.asarray(image.convert("RGB"), dtype=np.uint32)
    ids = rgb[:, :, 0] + 256 * rgb[:, :, 1] + 65536 * rgb[:, :, 2]
    valid = depth > 0
    assert np.array_equal(valid, ids > 0), "Depth foreground is misaligned with instance IDs"
    assert int(valid.sum()) == info["valid_pixels"]
    with Image.open(folder / info["preview"]) as image:
        preview = np.asarray(image)
    assert preview.shape == depth.shape
    expected = np.zeros(depth.shape, dtype=np.uint8)
    if valid.any():
        values = depth[valid].astype(np.float64)
        low, high = float(values.min()), float(values.max())
        assert np.isclose(low, info["min_valid"]) and np.isclose(high, info["max_valid"])
        assert low >= info["near_plane"] - 1e-5 and high <= info["far_plane"] + 1e-3
        normalized = (high - values) / (high - low) if high > low else np.ones_like(values)
        expected[valid] = (1 + np.floor(254 * np.clip(normalized, 0, 1) + 0.5)).astype(np.uint8)
    else:
        assert info["min_valid"] is None and info["max_valid"] is None
    assert np.array_equal(preview, expected), "Preview does not match numeric depth"
    print(f"PASS: {folder}; shape={depth.shape}, float32, top-left origin, background=0")
    print("Depth: positive camera-forward Z, in scene units")
    for item in sample["instances"]:
        values = depth[ids == item["instance_id"]]
        if values.size:
            print(f"instance {item['instance_id']}: pixels={values.size}, "
                  f"min={values.min():.6f}, median={np.median(values):.6f}, max={values.max():.6f}")
        else:
            print(f"instance {item['instance_id']}: no visible pixels")
    if pixel is not None:
        x, y = pixel
        if not (0 <= x < depth.shape[1] and 0 <= y < depth.shape[0]):
            raise ValueError("Pixel coordinates are outside this image")
        print(f"pixel(x={x},y={y}): instance={ids[y,x]}, depth={depth[y,x]:.7f}")
    return depth


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path)
    parser.add_argument("--pixel", nargs=2, type=int, metavar=("X", "Y"))
    args = parser.parse_args()
    inspect(args.folder, args.pixel)
