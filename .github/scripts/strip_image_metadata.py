#!/usr/bin/env python3
"""Remove EXIF/XMP metadata segments from JPEG files, leaving image data untouched.

Usage:
    python3 .github/scripts/strip_image_metadata.py <image.jpg> [...]

The rewritten files keep the exact same compressed image data, so the picture is
bit-for-bit identical once decoded - only the metadata segments (GPS position,
device model, timestamps, MakerNote, ...) are dropped.

Requires Pillow only for the verification step (pip install pillow).
"""

import sys
from pathlib import Path


def strip_jpeg(data: bytes) -> tuple[bytes, list[tuple[str, int]]]:
    if data[:2] != b"\xff\xd8":
        raise ValueError("not a JPEG")

    out = bytearray(data[:2])
    removed: list[tuple[str, int]] = []
    i = 2

    while i < len(data) - 1:
        if data[i] != 0xFF:
            raise ValueError(f"malformed JPEG at offset {i}")

        marker = data[i + 1]

        if marker in (0xD8, 0x01) or 0xD0 <= marker <= 0xD7:  # standalone markers
            out += data[i : i + 2]
            i += 2
            continue

        if marker == 0xDA:  # start of scan - copy the compressed data verbatim
            out += data[i:]
            break

        if marker == 0xD9:  # end of image
            out += data[i : i + 2]
            i += 2
            continue

        length = int.from_bytes(data[i + 2 : i + 4], "big")
        payload = data[i + 4 : i + 2 + length]

        is_exif = marker == 0xE1 and payload.startswith(b"Exif\x00\x00")
        is_xmp = marker == 0xE1 and payload.startswith(b"http://ns.adobe.com/xap/")
        if is_exif or is_xmp:
            removed.append((f"0x{marker:02x}", length + 2))
        else:
            out += data[i : i + 2 + length]

        i += 2 + length

    return bytes(out), removed


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 2

    for arg in argv:
        path = Path(arg)
        stripped, removed = strip_jpeg(path.read_bytes())
        if not removed:
            print(f"{path}: no EXIF/XMP segments, left unchanged")
            continue
        path.write_bytes(stripped)
        print(f"{path}: removed {len(removed)} metadata segment(s) {removed}")

        try:
            from PIL import Image

            exif = Image.open(path).getexif()
            gps = exif.get_ifd(0x8825) if exif else {}
            print(f"    verification: {len(exif)} exif tags left, GPS: {'PRESENT' if gps else 'none'}")
        except ImportError:
            print("    (install Pillow for automatic verification)")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
