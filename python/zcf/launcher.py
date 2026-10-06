from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import sys
from importlib.resources import files
from pathlib import Path


def _is_windows() -> bool:
    return sys.platform == "win32"


def _path_separator() -> str:
    return ";" if _is_windows() else ":"


def _application_name(name: str) -> str:
    return f"{name}.exe" if _is_windows() else name


def _native_binary() -> Path:
    name = _application_name("zcf")
    path = Path(str(files("zcf").joinpath("bin", name)))
    if not path.is_file():
        raise RuntimeError(f"Bundled zcf binary was not found: {path}")
    return path


def _run_native(*, clang_format_alias: bool) -> int:
    binary = _native_binary()
    environment = os.environ.copy()
    if clang_format_alias:
        environment["ZCF_INVOKED_AS_CLANG_FORMAT"] = "1"

    arguments = [
        "clang-format" if clang_format_alias else "zcf",
        *sys.argv[1:],
    ]
    if not _is_windows():
        os.execve(binary, arguments, environment)
        raise AssertionError("os.execve returned unexpectedly")

    completed = subprocess.run(
        [str(binary), *arguments[1:]],
        env=environment,
        check=False,
    )
    return completed.returncode


def _command_paths(name: str) -> list[Path]:
    suffixes = [""]
    if _is_windows() and not Path(name).suffix:
        suffixes.extend(
            suffix.lower()
            for suffix in os.environ.get("PATHEXT", ".EXE;.CMD;.BAT").split(";")
            if suffix
        )

    results: list[Path] = []
    seen: set[str] = set()
    for entry in os.environ.get("PATH", "").split(_path_separator()):
        if not entry:
            continue
        for suffix in suffixes:
            candidate = Path(entry) / f"{name}{suffix}"
            if not candidate.is_file() or (
                not _is_windows() and not os.access(candidate, os.X_OK)
            ):
                continue
            key = os.path.normcase(os.path.abspath(candidate))
            if key not in seen:
                seen.add(key)
                results.append(candidate)
    return results


def _pipx_executable() -> str:
    pipx = shutil.which("pipx")
    if not pipx:
        raise RuntimeError(
            "pipx was not found on PATH. Install pipx or run "
            "'pipx ensurepath --prepend' manually."
        )
    return pipx


def _managed_app_directory(pipx: str | None = None) -> Path:
    alias_name = _application_name("clang-format")

    configured_directory = os.environ.get("PIPX_BIN_DIR")
    if not configured_directory and pipx:
        completed = subprocess.run(
            [pipx, "environment", "--value", "PIPX_BIN_DIR"],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if completed.returncode == 0:
            configured_directory = completed.stdout.strip()

    candidates: list[Path] = []
    if configured_directory:
        candidates.append(Path(configured_directory).absolute())
    exposed_zcf = shutil.which("zcf")
    if exposed_zcf:
        candidates.append(Path(exposed_zcf).absolute().parent)

    for directory in candidates:
        zcf_name = _application_name("zcf")
        if (directory / zcf_name).is_file() and (
            directory / alias_name
        ).is_file():
            return directory

    searched = ", ".join(str(path) for path in candidates) or "<none>"
    raise RuntimeError(
        "The pipx-managed zcf and clang-format entry points were not found. "
        f"Searched: {searched}. Reinstall zcf with pipx."
    )


def _managed_alias(pipx: str | None = None) -> Path:
    directory = _managed_app_directory(pipx)
    alias_name = _application_name("clang-format")
    alias = directory / alias_name
    if not alias.is_file():
        raise RuntimeError(
            f"The pipx-managed clang-format entry point is missing: {alias}. "
            "Reinstall zcf with pipx."
        )
    return alias


def _same_path(left: Path, right: Path) -> bool:
    return os.path.normcase(os.path.abspath(left)) == os.path.normcase(
        os.path.abspath(right)
    )


def _print_clang_format_candidates() -> list[Path]:
    candidates = _command_paths("clang-format")
    if candidates:
        print("clang-format candidates:")
        for index, candidate in enumerate(candidates, start=1):
            marker = "active" if index == 1 else "shadowed"
            print(f"  {index}. {candidate} ({marker})")
    else:
        print("No clang-format command is currently discoverable on PATH.")
    return candidates


def _status() -> int:
    pipx = _pipx_executable()
    managed_alias = _managed_alias(pipx)
    candidates = _print_clang_format_candidates()
    if candidates and _same_path(candidates[0], managed_alias):
        print(f"zcf is the active clang-format: {managed_alias}")
        return 0

    print(f"zcf's managed clang-format is not active: {managed_alias}")
    print("Run 'zcf activate' and restart the terminal or IDE.")
    return 1


def _activate(*, dry_run: bool) -> int:
    pipx = _pipx_executable()
    managed_directory = _managed_app_directory(pipx)
    print(f"zcf pipx application directory: {managed_directory}")
    _print_clang_format_candidates()

    command = [pipx, "ensurepath", "--prepend"]
    if dry_run:
        command.append("--dry-run")
        print("Running preview: " + shlex.join(command))

    completed = subprocess.run(command, check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"pipx ensurepath failed with exit code {completed.returncode}."
        )

    if dry_run:
        return 0

    current_entries = os.environ.get("PATH", "").split(_path_separator())
    current_entries = [
        entry
        for entry in current_entries
        if entry and not _same_path(Path(entry), managed_directory)
    ]
    os.environ["PATH"] = _path_separator().join(
        [str(managed_directory), *current_entries]
    )

    print("zcf's pipx directory was prepended to PATH.")
    print("Restart running terminals and IDEs, then run 'zcf status'.")
    return _status()


def zcf_main() -> int:
    try:
        if len(sys.argv) >= 2 and sys.argv[1] == "activate":
            dry_run = len(sys.argv) == 3 and sys.argv[2] == "--dry-run"
            if len(sys.argv) > 2 and not dry_run:
                raise RuntimeError("Usage: zcf activate [--dry-run]")
            return _activate(dry_run=dry_run)
        if len(sys.argv) == 2 and sys.argv[1] == "status":
            return _status()
        return _run_native(clang_format_alias=False)
    except RuntimeError as error:
        print(f"zcf: error: {error}", file=sys.stderr)
        return 1


def clang_format_main() -> int:
    try:
        return _run_native(clang_format_alias=True)
    except RuntimeError as error:
        print(f"clang-format: error: {error}", file=sys.stderr)
        return 1
