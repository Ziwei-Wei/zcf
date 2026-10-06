from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


def run_git(repository: Path, *arguments: str) -> list[str]:
    command = ["git", "-C", str(repository), *arguments]
    completed = subprocess.run(
        command,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if completed.returncode != 0:
        output = completed.stdout.rstrip()
        raise RuntimeError(
            f"{' '.join(command)} failed"
            + (f":\n{output}" if output else "")
        )
    return completed.stdout.splitlines()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_arguments() -> argparse.Namespace:
    script_root = Path(__file__).resolve().parent
    repo_root = script_root.parent
    parser = argparse.ArgumentParser(
        description="Prepare the patched LLVM overlay used by zcf."
    )
    parser.add_argument(
        "--llvm-project-source-dir",
        type=Path,
        default=repo_root / "third_party" / "llvm-project",
    )
    parser.add_argument(
        "--overlay-dir",
        type=Path,
        default=repo_root / "build" / "llvm-overlay",
    )
    parser.add_argument(
        "--patch-dir",
        type=Path,
        default=repo_root / "integration" / "patches",
    )
    parser.add_argument(
        "--metadata-file",
        type=Path,
        default=repo_root / "build" / "llvm-overlay-metadata.json",
    )
    return parser.parse_args()


def overlay_is_current(
    overlay_dir: Path,
    metadata_file: Path,
    upstream_commit: str,
    patch_set_hash: str,
    patches: list[Path],
) -> bool:
    if not overlay_dir.is_dir() or not metadata_file.is_file():
        return False

    metadata = json.loads(metadata_file.read_text(encoding="utf-8"))
    if (
        metadata.get("upstreamCommit") != upstream_commit
        or metadata.get("patchSetHash") != patch_set_hash
    ):
        return False
    if run_git(overlay_dir, "rev-parse", "HEAD")[-1].strip() != upstream_commit:
        return False

    for patch in patches:
        completed = subprocess.run(
            [
                "git",
                "-C",
                str(overlay_dir),
                "apply",
                "--reverse",
                "--check",
                str(patch),
            ],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if completed.returncode != 0:
            return False

    status = run_git(
        overlay_dir, "status", "--porcelain", "--untracked-files=all"
    )
    if not status or any(not line.startswith(" M ") for line in status):
        return False

    current_diff = Path(f"{metadata_file}.current.patch")
    try:
        run_git(
            overlay_dir,
            "diff",
            "--no-ext-diff",
            "--src-prefix=a/",
            "--dst-prefix=b/",
            f"--output={current_diff}",
        )
        return sha256(current_diff) == sha256(patches[0])
    finally:
        current_diff.unlink(missing_ok=True)


def main() -> int:
    arguments = parse_arguments()
    llvm_source = arguments.llvm_project_source_dir.resolve()
    overlay_dir = arguments.overlay_dir.resolve()
    patch_dir = arguments.patch_dir.resolve()
    metadata_file = arguments.metadata_file.resolve()

    for required in (
        llvm_source / "llvm" / "CMakeLists.txt",
        llvm_source / "clang" / "lib" / "Format" / "Format.cpp",
    ):
        if not required.is_file():
            raise RuntimeError(f"LLVM source file was not found: {required}")
    if not patch_dir.is_dir():
        raise RuntimeError(
            f"Overlay patch directory was not found: {patch_dir}"
        )

    patches = sorted(patch_dir.glob("*.patch"))
    if len(patches) != 1:
        raise RuntimeError(
            f"Expected one policy-free bridge patch, found {len(patches)}."
        )

    source_status = run_git(llvm_source, "status", "--porcelain")
    if source_status:
        raise RuntimeError(
            "The LLVM source worktree must be clean before generating the "
            "overlay:\n" + "\n".join(source_status)
        )

    upstream_commit = run_git(llvm_source, "rev-parse", "HEAD")[-1].strip()
    patch_fingerprint = [
        f"{patch.name}:{sha256(patch).upper()}" for patch in patches
    ]
    patch_set_hash = hashlib.sha256(
        "\n".join(patch_fingerprint).encode("utf-8")
    ).hexdigest()

    if overlay_is_current(
        overlay_dir,
        metadata_file,
        upstream_commit,
        patch_set_hash,
        patches,
    ):
        print(f"LLVM overlay is current: {overlay_dir}")
        return 0

    if overlay_dir.exists():
        if (overlay_dir / ".git").exists():
            run_git(
                llvm_source,
                "worktree",
                "remove",
                "--force",
                str(overlay_dir),
            )
        else:
            shutil.rmtree(overlay_dir)
    run_git(llvm_source, "worktree", "prune")
    overlay_dir.parent.mkdir(parents=True, exist_ok=True)
    run_git(
        llvm_source,
        "worktree",
        "add",
        "--detach",
        str(overlay_dir),
        upstream_commit,
    )

    for patch in patches:
        run_git(overlay_dir, "apply", "--check", str(patch))
        run_git(overlay_dir, "apply", str(patch))

    metadata_file.parent.mkdir(parents=True, exist_ok=True)
    metadata_file.write_text(
        json.dumps(
            {
                "upstreamCommit": upstream_commit,
                "patchSetHash": patch_set_hash,
                "patches": patch_fingerprint,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    print(f"Prepared LLVM overlay: {overlay_dir}")
    print(f"Upstream commit: {upstream_commit}")
    print(f"Patch set: {patch_set_hash}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        raise SystemExit(f"error: {error}") from error
