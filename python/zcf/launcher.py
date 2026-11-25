from __future__ import annotations

import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.request
from importlib import metadata
from importlib.resources import files
from pathlib import Path

_PYPI_URL = "https://pypi.org/pypi/zcf/json"
_UPDATE_CHECK_INTERVAL_SECONDS = 24 * 60 * 60
_UPGRADE_COMMAND = "pipx upgrade zcf"
_VERSION = re.compile(r"(\d+)\.(\d+)\.(\d+)(?:(a|b|rc)(\d+))?")
_PRE_RELEASE_RANK = {"a": 0, "b": 1, "rc": 2}


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


def _skill_content() -> str:
    resource = files("zcf").joinpath("skills", "zcf", "SKILL.md")
    if not resource.is_file():
        raise RuntimeError(f"Packaged zcf skill was not found: {resource}")
    return resource.read_text(encoding="utf-8")


def _install_skill(repository: Path, *, force: bool) -> int:
    root = repository.expanduser().resolve()
    if not (root / ".git").exists():
        raise RuntimeError(f"Not a Git repository: {root}")

    destination = root / ".github" / "skills" / "zcf" / "SKILL.md"
    content = _skill_content()
    if destination.is_file():
        existing = destination.read_text(encoding="utf-8")
        if existing == content:
            print(f"zcf agent skill is already current: {destination}")
            return 0
        if not force:
            raise RuntimeError(
                f"Refusing to overwrite an existing skill: {destination}. "
                "Use 'zcf install-skill --force' to replace it."
            )

    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        newline="\n",
        dir=destination.parent,
        prefix=".SKILL.",
        suffix=".tmp",
        delete=False,
    ) as stream:
        stream.write(content)
        temporary = Path(stream.name)
    try:
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)
    print(f"Installed zcf agent skill: {destination}")
    return 0


def _parse_install_skill_arguments(arguments: list[str]) -> tuple[Path, bool]:
    force = False
    repository: Path | None = None
    for argument in arguments:
        if argument == "--force":
            force = True
        elif argument.startswith("-"):
            raise RuntimeError(
                "Usage: zcf install-skill [--force] [repository]"
            )
        elif repository is None:
            repository = Path(argument)
        else:
            raise RuntimeError(
                "Usage: zcf install-skill [--force] [repository]"
            )
    return repository or Path.cwd(), force


def _installed_version() -> str | None:
    try:
        return metadata.version("zcf")
    except metadata.PackageNotFoundError:
        return None


def _version_key(version: str) -> tuple[int, ...] | None:
    match = _VERSION.fullmatch(version.strip())
    if not match:
        return None
    major, minor, patch, pre_kind, pre_number = match.groups()
    if pre_kind is None:
        return (int(major), int(minor), int(patch), 1, 0, 0)
    return (
        int(major),
        int(minor),
        int(patch),
        0,
        _PRE_RELEASE_RANK[pre_kind],
        int(pre_number),
    )


def _is_newer(candidate: str, installed: str) -> bool:
    candidate_key = _version_key(candidate)
    installed_key = _version_key(installed)
    return (
        candidate_key is not None
        and installed_key is not None
        and candidate_key > installed_key
    )


def _latest_version(*, timeout: float) -> str:
    with urllib.request.urlopen(_PYPI_URL, timeout=timeout) as response:
        version = json.load(response)["info"]["version"]
    if not isinstance(version, str):
        raise ValueError("PyPI returned no version for zcf")
    return version


def _update_cache_file() -> Path:
    if _is_windows():
        base = os.environ.get("LOCALAPPDATA") or str(
            Path.home() / "AppData" / "Local"
        )
    elif sys.platform == "darwin":
        base = str(Path.home() / "Library" / "Caches")
    else:
        base = os.environ.get("XDG_CACHE_HOME") or str(Path.home() / ".cache")
    return Path(base) / "zcf" / "update-check.json"


def _write_update_cache(latest: str | None) -> None:
    cache = _update_cache_file()
    cache.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        dir=cache.parent,
        prefix=".update-check.",
        suffix=".tmp",
        delete=False,
    ) as stream:
        json.dump({"checked": time.time(), "latest": latest}, stream)
        temporary = Path(stream.name)
    try:
        os.replace(temporary, cache)
    finally:
        temporary.unlink(missing_ok=True)


def _upgrade_message(installed: str, latest: str) -> str:
    return (
        f"zcf {latest} is available; this is zcf {installed}. "
        f"Update with '{_UPGRADE_COMMAND}', then run the command again."
    )


# Runs before formatting. It must never fail or slow down a run: PyPI is
# asked at most once per interval, failures are cached too, and the notice
# goes to stderr so formatted output on stdout is unchanged.
def _notify_if_outdated() -> None:
    if os.environ.get("ZCF_NO_UPDATE_CHECK") or os.environ.get("CI"):
        return
    installed = _installed_version()
    if installed is None:
        return

    latest = None
    checked = 0.0
    try:
        state = json.loads(_update_cache_file().read_text(encoding="utf-8"))
        latest = state.get("latest")
        checked = float(state.get("checked", 0))
    except (OSError, ValueError, TypeError, AttributeError):
        pass

    if time.time() - checked >= _UPDATE_CHECK_INTERVAL_SECONDS:
        try:
            latest = _latest_version(timeout=2)
        except (OSError, ValueError, KeyError, TypeError):
            pass
        try:
            _write_update_cache(latest)
        except OSError:
            pass

    if isinstance(latest, str) and _is_newer(latest, installed):
        print(f"zcf: {_upgrade_message(installed, latest)}", file=sys.stderr)


def _check_update() -> int:
    installed = _installed_version()
    if installed is None:
        raise RuntimeError("zcf is not installed as a package.")
    try:
        latest = _latest_version(timeout=10)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(
            f"zcf: could not get the latest version from PyPI: {error}",
            file=sys.stderr,
        )
        return 2
    try:
        _write_update_cache(latest)
    except OSError:
        pass

    if _is_newer(latest, installed):
        print(_upgrade_message(installed, latest))
        return 1
    print(f"zcf {installed} is the latest version.")
    return 0


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
        if len(sys.argv) >= 2 and sys.argv[1] == "install-skill":
            repository, force = _parse_install_skill_arguments(sys.argv[2:])
            return _install_skill(repository, force=force)
        if len(sys.argv) == 2 and sys.argv[1] == "check-update":
            return _check_update()
        _notify_if_outdated()
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
