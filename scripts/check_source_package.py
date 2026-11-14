"""Extract a draft package into a new directory; verify hashes and asset references.

python scripts/check_source_package.py archive.zip NEW_directory [--verify-dataset]
"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path


def check(archive_path, target, verify=False):
    target = target.resolve()
    if target.exists():
        raise ValueError("Choose a new extraction directory")
    with zipfile.ZipFile(archive_path) as archive:
        assert all((target / name).resolve().is_relative_to(target) for name in archive.namelist())
        assert archive.testzip() is None
        archive.extractall(target)
    root = target / "CGAssignment2"
    manifest = json.loads((root / "PACKAGE_MANIFEST.json").read_text(encoding="utf-8"))
    for item in manifest["files"]:
        path = (root / item["path"]).resolve()
        assert path.is_relative_to(root)
        assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"]
    scenes = 0
    for path in root.rglob("*.json"):
        data = json.loads(path.read_text(encoding="utf-8"))
        if data.get("format") != "CGAssignment1":
            continue
        scenes += 1
        for item in data["objects"]:
            for name in (item.get("model", ""), item["material"].get("texture", "")):
                if name:
                    asset = (path.parent / name).resolve()
                    assert asset.is_relative_to(root) and asset.is_file(), (path, name)
    assert not (root / "out").exists() and not (root / "report").exists()
    print(f"PACKAGE PASS: {len(manifest['files'])} hashes, {scenes} scenes with included assets")
    if verify and (root / "examples/dataset").is_dir():
        sys.path.insert(0, str(root / "scripts"))
        from verify_dataset import verify_dataset
        verify_dataset(root / "examples/dataset")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("new_directory", type=Path)
    parser.add_argument("--verify-dataset", action="store_true")
    args = parser.parse_args()
    check(args.archive, args.new_directory, args.verify_dataset)
