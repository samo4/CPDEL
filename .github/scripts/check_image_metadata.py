#!/usr/bin/env python3
"""Fail if any tracked image carries GPS (or other identifying) metadata.

Photos taken on a phone keep EXIF by default, and pandoc/LaTeX copies the whole
JPEG - including the EXIF block - into the generated PDF.  That is how precise
coordinates end up in a public repository and in release assets.
"""

import subprocess
import sys
from pathlib import Path

from PIL import Image, ExifTags

IMAGE_EXT = {".jpg", ".jpeg", ".png", ".tif", ".tiff", ".webp"}
GPS_IFD = 0x8825
# Tags that are not fatal on their own, but worth knowing about.
NOTABLE_TAGS = {"Make", "Model", "HostComputer", "DateTime", "DateTimeOriginal", "Artist", "Copyright"}


def tracked_images() -> list[Path]:
    raw = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True).stdout
    return [Path(p) for p in raw.decode("utf-8", "surrogateescape").split("\0") if p]


def main() -> int:
    violations: list[str] = []
    warnings: list[str] = []

    for path in tracked_images():
        if path.suffix.lower() not in IMAGE_EXT or not path.is_file():
            continue

        try:
            exif = Image.open(path).getexif()
        except Exception as exc:  # noqa: BLE001
            warnings.append(f"{path}: could not read image ({exc})")
            continue

        if not exif:
            continue

        gps = exif.get_ifd(GPS_IFD) or {}
        if gps:
            found = {
                ExifTags.GPSTAGS.get(k, hex(k)): v
                for k, v in gps.items()
                if "Ref" not in ExifTags.GPSTAGS.get(k, "")
            }
            violations.append(f"{path}: GPS metadata present -> {found}")

        notable = {ExifTags.TAGS.get(k, hex(k)): v for k, v in exif.items() if ExifTags.TAGS.get(k) in NOTABLE_TAGS}
        if notable:
            warnings.append(f"{path}: identifying metadata -> {notable}")

    for line in warnings:
        print(f"::warning::{line}")

    if violations:
        print()
        for line in violations:
            print(f"::error::{line}")
        print()
        print("Strip the metadata before committing, e.g.:")
        print("  exiftool -all= <image>            # or")
        print("  pandoc ... (rebuild) - see .github/scripts/strip_image_metadata.py")
        return 1

    print("No GPS metadata found in tracked images.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
