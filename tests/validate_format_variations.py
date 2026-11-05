"""Checks zcf invariants over the contract inputs under style variations.

Every contract input is formatted with each base style and indentation
variation, once with LF and once with CRLF line endings, and once with a
`-lines` range covering its middle third. With every extension enabled, zcf
must not crash or print diagnostics, must be stable across a second run, may
change only whitespace and integer-literal case relative to the same variation
with every extension disabled, must keep CRLF output free of bare LF line
endings, and must not edit lines outside a `-lines` range. A problem that also
occurs with extensions disabled is reported as upstream and does not fail the
check.
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
INPUT_GLOBS = (
    "tests/zcf-style-contracts/cases/*.input.cpp",
    "tests/cpp-style-examples/*.input.cpp",
)
VARIATIONS = {
    "zcf": "BasedOnStyle: ZCF",
    "zcf-indent2": "BasedOnStyle: ZCF, IndentWidth: 2, ContinuationIndentWidth: 2",
    "zcf-indent8": "BasedOnStyle: ZCF, IndentWidth: 8, ContinuationIndentWidth: 8",
    "zcf-tabs": "BasedOnStyle: ZCF, UseTab: ForIndentation, TabWidth: 4",
    "zcf-tabs-always": "BasedOnStyle: ZCF, UseTab: Always, TabWidth: 4",
    "zcf-col0": "BasedOnStyle: ZCF, ColumnLimit: 0",
    "zcf-col60": "BasedOnStyle: ZCF, ColumnLimit: 60",
    "llvm": "BasedOnStyle: LLVM",
    "google": "BasedOnStyle: Google",
    "microsoft": "BasedOnStyle: Microsoft",
    "webkit": "BasedOnStyle: WebKit",
}
# Inputs shorter than this have no meaningful middle third for `-lines`.
MIN_RANGE_LINES = 9
NUMBER = re.compile(rb"(?<![\w.])\.?\d[\w'.]*")
BARE_LF = re.compile(rb"(?<!\r)\n")


def extension_options() -> list[str]:
    definitions = (
        REPOSITORY / "extensions" / "clang-format" / "ExtensionOptions.def"
    ).read_text(encoding="utf-8")
    return re.findall(r"^ZCF_EXTENSION_OPTION\((\w+)\)$", definitions, re.M)


def build_styles(options: list[str]) -> dict[str, tuple[str, str]]:
    def extensions(value: str) -> str:
        return "ZCFExtensions: {" + ", ".join(f"{o}: {value}" for o in options) + "}"

    return {
        name: (
            "{" + base + ", " + extensions("true") + "}",
            "{" + base + ", " + extensions("false") + "}",
        )
        for name, base in VARIATIONS.items()
    }


@dataclass
class Result:
    path: str
    variation: str
    extension_issues: list[str] = field(default_factory=list)
    upstream_issues: list[str] = field(default_factory=list)


def format_text(
    formatter: Path, style: str, text: bytes, *arguments: str
) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        [str(formatter), f"-style={style}", "-assume-filename=input.cpp", *arguments],
        input=text,
        capture_output=True,
        check=False,
    )


def failed(run: subprocess.CompletedProcess[bytes]) -> bool:
    return run.returncode != 0 or bool(run.stderr.strip())


def normalized_tokens(text: bytes) -> bytes:
    # Line continuations and the `//` markers of rewrapped line comments are
    # layout, not code.
    text = re.sub(rb"\\\r?\n", b"", text).replace(b"//", b"")
    return re.sub(rb"\s+", b"", NUMBER.sub(lambda m: m.group(0).lower(), text))


def check_full_format(
    formatter: Path, styles: tuple[str, str], text: bytes, label: str, result: Result
) -> None:
    on_style, off_style = styles
    first = format_text(formatter, on_style, text)
    base = format_text(formatter, off_style, text)

    def report(issue: str, upstream_too: bool) -> None:
        target = result.upstream_issues if upstream_too else result.extension_issues
        target.append(f"{label}:{issue}")

    if failed(first):
        report("crash" if first.returncode != 0 else "diagnostics", failed(base))
        return

    second = format_text(formatter, on_style, first.stdout)
    if failed(second) or second.stdout != first.stdout:
        base_second = (
            None if failed(base) else format_text(formatter, off_style, base.stdout)
        )
        report(
            "not-idempotent",
            base_second is None or base_second.stdout != base.stdout,
        )

    if not failed(base) and normalized_tokens(first.stdout) != normalized_tokens(
        base.stdout
    ):
        result.extension_issues.append(f"{label}:token-change")

    if label == "crlf" and BARE_LF.search(first.stdout):
        report("bare-lf", failed(base) or bool(BARE_LF.search(base.stdout)))


def check_line_range(
    formatter: Path, styles: tuple[str, str], text: bytes, result: Result
) -> None:
    lines = text.split(b"\n")
    if len(lines) < MIN_RANGE_LINES:
        return
    begin, end = len(lines) // 3 + 1, 2 * len(lines) // 3
    head, tail = lines[: begin - 1], lines[end:]

    def run(style: str) -> subprocess.CompletedProcess[bytes]:
        return format_text(formatter, style, text, f"-lines={begin}:{end}")

    def edits_outside(output: bytes) -> bool:
        out = output.split(b"\n")
        return out[: len(head)] != head or (bool(tail) and out[-len(tail) :] != tail)

    on_style, off_style = styles
    ranged = run(on_style)
    base = run(off_style)
    if failed(ranged):
        target = result.upstream_issues if failed(base) else result.extension_issues
        target.append("lines:crash" if ranged.returncode != 0 else "lines:diagnostics")
    elif edits_outside(ranged.stdout):
        # Upstream edits outside the range too when it starts mid-statement.
        upstream_too = failed(base) or edits_outside(base.stdout)
        target = result.upstream_issues if upstream_too else result.extension_issues
        target.append("lines:edits-outside-range")


def check_file(
    formatter: Path, path: Path, variation: str, styles: tuple[str, str]
) -> Result:
    shown = path.relative_to(REPOSITORY) if path.is_relative_to(REPOSITORY) else path
    result = Result(str(shown), variation)
    text = path.read_bytes().replace(b"\r\n", b"\n")
    check_full_format(formatter, styles, text, "lf", result)
    check_full_format(formatter, styles, text.replace(b"\n", b"\r\n"), "crlf", result)
    check_line_range(formatter, styles, text, result)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--formatter", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--report", type=Path)
    parser.add_argument(
        "--files", nargs="*", type=Path, help="Check these files instead of the contract inputs."
    )
    parser.add_argument(
        "--variations",
        nargs="*",
        choices=sorted(VARIATIONS),
        help="Check only these variations.",
    )
    arguments = parser.parse_args()

    styles = build_styles(extension_options())
    if arguments.variations:
        styles = {name: styles[name] for name in arguments.variations}
    files = [path.resolve() for path in arguments.files or []] or sorted(
        path for pattern in INPUT_GLOBS for path in REPOSITORY.glob(pattern)
    )
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

    print(
        f"Checked {len(files)} inputs with {len(styles)} variations "
        "(LF, CRLF, and -lines)."
    )
    failures = 0
    for result in results:
        for issue in result.extension_issues:
            print(f"EXTENSION {issue:26} {result.variation:15} {result.path}")
            failures += 1
    # Upstream `-lines` formatting edits whole statements around a range that
    # starts mid-statement, so upstream issues are summarized; --report lists
    # them.
    upstream: dict[str, int] = {}
    for result in results:
        for issue in result.upstream_issues:
            upstream[issue] = upstream.get(issue, 0) + 1
    for issue, count in sorted(upstream.items()):
        print(f"UPSTREAM  {issue:26} {count} inputs and variations")
    print(f"{failures} extension-caused issues.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
