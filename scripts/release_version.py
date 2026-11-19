"""Resolve, validate, and apply the package version for a release build."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

PYPROJECT = Path(__file__).resolve().parent.parent / "pyproject.toml"
VERSION_LINE = re.compile(r'^version = "([^"]+)"$', re.MULTILINE)
RELEASE_VERSION = re.compile(r"^\d+\.\d+\.\d+((a|b|rc)\d+)?$")


def project_version(text: str) -> str:
    match = VERSION_LINE.search(text)
    if match is None:
        raise ValueError(f"{PYPROJECT} has no version line")
    return match.group(1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--version",
        default="",
        help="release version; defaults to the version in pyproject.toml",
    )
    parser.add_argument(
        "--tag",
        default="",
        help="release tag that must equal v<version>",
    )
    parser.add_argument(
        "--write",
        action="store_true",
        help="write the version into pyproject.toml",
    )
    arguments = parser.parse_args()

    text = PYPROJECT.read_text(encoding="utf-8")
    version = arguments.version.strip() or project_version(text)
    if not RELEASE_VERSION.match(version):
        raise ValueError(
            f"release version {version!r} must look like 1.2.3, 1.2.3rc1, "
            "1.2.3a1, or 1.2.3b1"
        )
    if arguments.tag and arguments.tag != f"v{version}":
        raise ValueError(f"tag {arguments.tag} does not match v{version}")
    if arguments.write:
        PYPROJECT.write_text(
            VERSION_LINE.sub(f'version = "{version}"', text, count=1),
            encoding="utf-8",
        )

    print(version)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
