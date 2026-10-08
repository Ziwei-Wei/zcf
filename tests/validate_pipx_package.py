from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import sysconfig
import tempfile
from pathlib import Path


def run(
    arguments: list[str],
    *,
    environment: dict[str, str],
    input_text: str | None = None,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        arguments,
        check=False,
        env=environment,
        input=input_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"{' '.join(arguments)} failed with {completed.returncode}:\n"
            f"{completed.stdout}"
        )
    return completed


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate a zcf wheel through an isolated pipx install."
    )
    parser.add_argument("--wheel", type=Path)
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument(
        "--activate",
        action="store_true",
        help="Run the real pipx PATH activation instead of its dry run.",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    repo_root = Path(__file__).resolve().parents[1]
    wheel = arguments.wheel
    if wheel is None:
        wheels = sorted((repo_root / "build" / "dist").glob("zcf-*.whl"))
        if len(wheels) != 1:
            raise RuntimeError(
                "Expected one zcf wheel under build/dist; found "
                f"{len(wheels)}."
            )
        wheel = wheels[0]
    wheel = wheel.resolve()
    if not wheel.is_file():
        raise RuntimeError(f"zcf wheel was not found: {wheel}")

    temporary = None
    if arguments.work_dir:
        work_dir = arguments.work_dir.resolve()
        shutil.rmtree(work_dir, ignore_errors=True)
        work_dir.mkdir(parents=True)
    else:
        temporary = tempfile.TemporaryDirectory(prefix="zcf-pipx-")
        work_dir = Path(temporary.name)

    pipx_home = work_dir / "home"
    pipx_bin = work_dir / "bin"
    environment = os.environ.copy()
    environment["PIPX_HOME"] = str(pipx_home)
    environment["PIPX_BIN_DIR"] = str(pipx_bin)
    environment["HOME"] = str(work_dir / "profile")
    environment["PATH"] = os.pathsep.join(
        [
            str(pipx_bin),
            sysconfig.get_path("scripts"),
            environment.get("PATH", ""),
        ]
    )

    executable_suffix = ".exe" if os.name == "nt" else ""
    zcf = pipx_bin / f"zcf{executable_suffix}"
    clang_format = pipx_bin / f"clang-format{executable_suffix}"

    try:
        run(
            [sys.executable, "-m", "pipx", "install", str(wheel), "--force"],
            environment=environment,
        )
        for application in (zcf, clang_format):
            if not application.is_file():
                raise RuntimeError(
                    f"pipx did not expose the expected application: "
                    f"{application}"
                )

        zcf_version = run(
            [str(zcf), "--version"], environment=environment
        ).stdout
        if not zcf_version.startswith("zcf (clang-format version "):
            raise RuntimeError(
                f"zcf reported an unexpected version: {zcf_version}"
            )
        clang_version = run(
            [str(clang_format), "--version"], environment=environment
        ).stdout
        if not clang_version.startswith("clang-format version "):
            raise RuntimeError(
                "clang-format reported an unexpected version: "
                f"{clang_version}"
            )

        status = run([str(zcf), "status"], environment=environment).stdout
        if "zcf is the active clang-format" not in status:
            raise RuntimeError(
                f"zcf status did not identify the pipx alias: {status}"
            )

        repository = work_dir / "repository"
        (repository / ".git").mkdir(parents=True)
        installation = run(
            [str(zcf), "install-skill", str(repository)],
            environment=environment,
        ).stdout
        skill = repository / ".github" / "skills" / "zcf" / "SKILL.md"
        if not skill.is_file() or "name: zcf" not in skill.read_text(
            encoding="utf-8"
        ):
            raise RuntimeError(
                f"zcf did not install its agent skill: {installation}"
            )
        idempotent = run(
            [str(zcf), "install-skill", str(repository)],
            environment=environment,
        ).stdout
        if "already current" not in idempotent:
            raise RuntimeError(
                f"zcf skill reinstall was not idempotent: {idempotent}"
            )
        skill.write_text("user-owned\n", encoding="utf-8")
        conflict = subprocess.run(
            [str(zcf), "install-skill", str(repository)],
            check=False,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        if conflict.returncode == 0 or "Refusing to overwrite" not in conflict.stdout:
            raise RuntimeError(
                "zcf did not refuse a conflicting skill installation:\n"
                f"{conflict.stdout}"
            )
        run(
            [
                str(zcf),
                "install-skill",
                "--force",
                str(repository),
            ],
            environment=environment,
        )
        if "name: zcf" not in skill.read_text(encoding="utf-8"):
            raise RuntimeError("zcf --force did not restore the packaged skill.")

        existing_dir = work_dir / "existing"
        existing_dir.mkdir()
        existing_alias = existing_dir / clang_format.name
        shutil.copy2(clang_format, existing_alias, follow_symlinks=True)
        conflict_environment = environment.copy()
        conflict_environment["PATH"] = os.pathsep.join(
            [str(existing_dir), environment["PATH"]]
        )
        activation_arguments = [str(zcf), "activate"]
        if not arguments.activate:
            activation_arguments.append("--dry-run")
        activation = run(
            activation_arguments,
            environment=conflict_environment,
        ).stdout
        expected_activation = (
            "zcf is the active clang-format"
            if arguments.activate
            else "ensurepath --prepend --dry-run"
        )
        if (
            str(existing_alias) not in activation
            or expected_activation not in activation
        ):
            raise RuntimeError(
                "zcf activation did not report and safely shadow the "
                f"existing formatter:\n{activation}"
            )

        source = work_dir / "sample.cpp"
        source.write_text("int main(){return 0;}\n", encoding="utf-8")
        formatted = run(
            [str(clang_format), "-style=ZCF", str(source)],
            environment=environment,
        ).stdout
        if "int\nmain()" not in formatted.replace("\r\n", "\n"):
            raise RuntimeError(
                f"pipx clang-format did not apply ZCF:\n{formatted}"
            )

        run(
            [sys.executable, "-m", "pipx", "uninstall", "zcf"],
            environment=environment,
        )
        for application in (zcf, clang_format):
            if application.exists():
                raise RuntimeError(
                    f"pipx uninstall left an application behind: "
                    f"{application}"
                )

        print("zcf pipx package tests passed")
        return 0
    finally:
        shutil.rmtree(work_dir, ignore_errors=True)
        if temporary:
            temporary.cleanup()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        raise SystemExit(f"error: {error}") from error
