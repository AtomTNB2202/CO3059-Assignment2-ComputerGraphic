"""Create a draft source archive; no build caches, old report, or local environments.

python scripts/package_assignment2.py --evidence output/milestone4/batch_seed42_a
"""
import argparse
import hashlib
import json
import posixpath
import zipfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRS = ("include", "src", "shaders", "assets", "tests", "scripts", "docs", "third_party", "screenshots")
SOURCE_FILES = ("CMakeLists.txt", "README.md", "THIRD_PARTY.md", "SUBMISSION_STATUS.md", ".gitignore", ".clang-format", ".gitattributes")


def asset_reference(name, source_json, archive_json):
    if not name:
        return ""
    asset = Path(name)
    if not asset.is_absolute():
        asset = source_json.parent / asset
    asset = asset.resolve()
    # Build-copied assets have the same assets/... suffix as project assets.
    for i, part in reversed(list(enumerate(asset.parts))):
        if part == "assets":
            relative = PurePosixPath(*asset.parts[i:])
            original = ROOT / Path(relative)
            if original.is_file() and asset.is_file() and original.read_bytes() == asset.read_bytes():
                return posixpath.relpath(str(relative), str(archive_json.parent))
    raise ValueError(f"Evidence references an asset outside the included project assets: {asset}")


def portable_evidence(source, archive_path):
    data = source.read_bytes()
    if source.suffix != ".json":
        return data
    obj = json.loads(data)
    convert = lambda name: asset_reference(name, source, archive_path)
    if obj.get("format") == "CGAssignment1":
        for item in obj["objects"]:
            if "model" in item:
                item["model"] = convert(item["model"])
            item["material"]["texture"] = convert(item["material"]["texture"])
    if "instances" in obj:
        for item in obj["instances"]:
            item["model_path"] = convert(item.get("model_path", ""))
            item["material"]["texture_path"] = convert(item["material"].get("texture_path", ""))
    for config in (obj, obj.get("settings", {})):
        if "texture_pool" in config:
            config["texture_pool"] = [convert(name) for name in config["texture_pool"]]
    return (json.dumps(obj, indent=2, ensure_ascii=False) + "\n").encode("utf-8")


def package(output, evidence=None):
    output = output.resolve()
    partial = output.with_name(output.name + ".partial")
    if output.exists() or partial.exists():
        raise ValueError("Choose a new archive filename; existing output is not overwritten")
    files = {}
    for folder in SOURCE_DIRS:
        for source in sorted((ROOT / folder).rglob("*")):
            if source.is_file() and "__pycache__" not in source.parts and source.suffix != ".pyc":
                files[source.relative_to(ROOT).as_posix()] = source.read_bytes()
    for name in SOURCE_FILES:
        files[name] = (ROOT / name).read_bytes()
    if evidence is not None:
        evidence = evidence.resolve()
        if not (evidence / "dataset.json").is_file():
            raise ValueError("Evidence must be a complete batch directory containing dataset.json")
        for source in sorted(evidence.rglob("*")):
            if source.is_file():
                archived = PurePosixPath("examples/dataset") / source.relative_to(evidence).as_posix()
                files[str(archived)] = portable_evidence(source, archived)
    manifest = {"format": "CGAssignment2SourceDraft", "version": 1, "final_report_included": False,
                "evidence_included": evidence is not None,
                "files": [{"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                          for name, data in sorted(files.items())]}
    files["PACKAGE_MANIFEST.json"] = (json.dumps(manifest, indent=2) + "\n").encode("utf-8")
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(partial, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            archive.writestr("CGAssignment2/" + name, data)
    with zipfile.ZipFile(partial) as archive:
        if archive.testzip() is not None:
            raise ValueError("Archive integrity check failed")
        for item in manifest["files"]:
            assert hashlib.sha256(archive.read("CGAssignment2/" + item["path"])).hexdigest() == item["sha256"]
        assert not any("/out/" in name or "/report/" in name or "__pycache__" in name for name in archive.namelist())
    partial.rename(output)
    print(f"DRAFT PACKAGE: {output} ({len(files)} files, {output.stat().st_size} bytes)")
    print("Final Assignment 2 TEX/PDF is still pending; see SUBMISSION_STATUS.md for scope.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "output/submission/CGAssignment2-source-draft.zip")
    parser.add_argument("--evidence", type=Path)
    args = parser.parse_args()
    package(args.output, args.evidence)
