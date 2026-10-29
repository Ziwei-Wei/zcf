"""Checks zcf invariants over a fixed sample of the pinned LLVM sources.

For every sampled file and style, zcf with extensions enabled must not crash
or print diagnostics, must be stable across a second run, and may change only
whitespace and integer-literal case relative to the same style with every
extension disabled. A problem that also occurs with extensions disabled is
reported as upstream and does not fail the check.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parent.parent
CORPUS_ROOTS = (
    Path("clang/lib"),
    Path("llvm/lib"),
    Path("libcxx/include"),
)
SOURCE_SUFFIXES = {".cpp", ".h", ".hpp"}
MAX_FILE_BYTES = 150_000
NUMBER = re.compile(rb"(?<![\w.])\.?\d[\w'.]*")


def extension_options() -> list[str]:
    definitions = (
        REPOSITORY / "extensions" / "clang-format" / "ExtensionOptions.def"
    ).read_text(encoding="utf-8")
    return re.findall(r"^ZCF_EXTENSION_OPTION\((\w+)\)$", definitions, re.M)


def build_styles(options: list[str]) -> dict[str, tuple[str, str]]:
    def extensions(value: str) -> str:
        return ", ".join(f"{option}: {value}" for option in options)

    off = "ZCFExtensions: {" + extensions("false") + "}"
    on = "ZCFExtensions: {" + extensions("true") + "}"
    tabs = "UseTab: ForIndentation, TabWidth: 4, ColumnLimit: 0"
    return {
        "zcf": ("{BasedOnStyle: ZCF}", "{BasedOnStyle: ZCF, " + off + "}"),
        "llvm-all": ("{BasedOnStyle: LLVM, " + on + "}", "{BasedOnStyle: LLVM}"),
        "zcf-tabs-col0": (
            "{BasedOnStyle: ZCF, " + tabs + "}",
            "{BasedOnStyle: ZCF, " + tabs + ", " + off + "}",
        ),
    }


def sample_corpus(llvm_source: Path, limit: int) -> list[Path]:
    files: list[Path] = []
    for root in CORPUS_ROOTS:
        for path in (llvm_source / root).rglob("*"):
            if not path.is_file() or path.stat().st_size > MAX_FILE_BYTES:
                continue
            # libc++ public headers have no suffix.
            if path.suffix in SOURCE_SUFFIXES or (
                root.parts[0] == "libcxx" and not path.suffix
            ):
                files.append(path)
    files.sort()
    step = max(1, len(files) // limit)
    return files[::step][:limit]


@dataclass
class Result:
    path: str
    style: str
    extension_issues: list[str] = field(default_factory=list)
    upstream_issues: list[str] = field(default_factory=list)


def format_text(
    formatter: Path, style: str, text: bytes, name: Path
) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        [str(formatter), f"-style={style}", f"-assume-filename={name}"],
        input=text,
        capture_output=True,
        check=False,
    )


def failed(run: subprocess.CompletedProcess[bytes]) -> bool:
    return run.returncode != 0 or bool(run.stderr.strip())


def normalized_tokens(text: bytes) -> bytes:
    return re.sub(rb"\s+", b"", NUMBER.sub(lambda m: m.group(0).lower(), text))


def check_file(
    formatter: Path, path: Path, style_name: str, styles: tuple[str, str]
) -> Result:
    on_style, off_style = styles
    result = Result(str(path), style_name)
    text = path.read_bytes()

    first = format_text(formatter, on_style, text, path)
    base = format_text(formatter, off_style, text, path)
    if failed(first):
        target = result.upstream_issues if failed(base) else result.extension_issues
        target.append("crash" if first.returncode != 0 else "diagnostics")
        return result

    second = format_text(formatter, on_style, first.stdout, path)
    if failed(second) or second.stdout != first.stdout:
        base_second = (
            None if failed(base) else format_text(formatter, off_style, base.stdout, path)
        )
        upstream_unstable = base_second is None or base_second.stdout != base.stdout
        target = result.upstream_issues if upstream_unstable else result.extension_issues
        target.append("not-idempotent")

    if not failed(base) and normalized_tokens(first.stdout) != normalized_tokens(
        base.stdout
    ):
        result.extension_issues.append("token-change")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--formatter", type=Path, required=True)
    parser.add_argument(
        "--llvm-source",
        type=Path,
        default=REPOSITORY / "third_party" / "llvm-project",
    )
    parser.add_argument("--limit", type=int, default=600)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--report", type=Path)
    parser.add_argument(
        "--files", nargs="*", type=Path, help="Check these files instead of a sample."
    )
    arguments = parser.parse_args()

    styles = build_styles(extension_options())
    files = arguments.files or sample_corpus(arguments.llvm_source, arguments.limit)
    with concurrent.futures.ThreadPoolExecutor(max_workers=arguments.jobs) as pool:
        futures = [
            pool.submit(check_file, arguments.formatter, path, name, pair)
            for path in files
            for name, pair in styles.items()
        ]
        results = [future.result() for future in futures]

    if arguments.report:
        arguments.report.parent.mkdir(parents=True, exist_ok=True)
        arguments.report.write_text(
            json.dumps([result.__dict__ for result in results], indent=1),
            encoding="utf-8",
        )

    print(f"Checked {len(files)} files with {len(styles)} styles.")
    failures = 0
    for kind in ("extension", "upstream"):
        for result in results:
            for issue in getattr(result, f"{kind}_issues"):
                print(f"{kind.upper():9} {issue:15} {result.style:14} {result.path}")
                failures += kind == "extension"
    print(f"{failures} extension-caused issues.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
