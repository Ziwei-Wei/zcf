from __future__ import annotations

import os
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).parents[1] / "python"))

from zcf import launcher


class LauncherTests(unittest.TestCase):
    def test_install_skill_is_idempotent_and_refuses_conflicts(self):
        content = (
            "---\n"
            "name: zcf\n"
            "description: Test skill\n"
            "---\n"
            "\n"
            "# zcf\n"
        )
        with tempfile.TemporaryDirectory() as temporary:
            repository = Path(temporary)
            (repository / ".git").mkdir()
            destination = (
                repository
                / ".github"
                / "skills"
                / "zcf"
                / "SKILL.md"
            )

            with mock.patch.object(
                launcher,
                "_skill_content",
                return_value=content,
            ):
                self.assertEqual(
                    launcher._install_skill(repository, force=False),
                    0,
                )
                self.assertEqual(destination.read_text(), content)
                self.assertEqual(
                    launcher._install_skill(repository, force=False),
                    0,
                )

                destination.write_text("user-owned\n", encoding="utf-8")
                with self.assertRaisesRegex(
                    RuntimeError,
                    "Refusing to overwrite",
                ):
                    launcher._install_skill(repository, force=False)

                self.assertEqual(
                    launcher._install_skill(repository, force=True),
                    0,
                )
                self.assertEqual(destination.read_text(), content)

    def test_install_skill_requires_git_repository(self):
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(RuntimeError, "Not a Git repository"):
                launcher._install_skill(Path(temporary), force=False)

    def test_activate_uses_pipx_prepend_and_reports_existing_formatters(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            managed = root / "managed"
            existing = root / "existing"
            managed.mkdir()
            existing.mkdir()

            zcf_name = launcher._application_name("zcf")
            alias_name = launcher._application_name("clang-format")
            pipx_name = launcher._application_name("pipx")
            for path in (
                managed / zcf_name,
                managed / alias_name,
                existing / alias_name,
                root / pipx_name,
            ):
                path.write_bytes(b"test")
                if not launcher._is_windows():
                    path.chmod(0o755)

            def which(name: str):
                return {
                    "zcf": str(managed / zcf_name),
                    "pipx": str(root / pipx_name),
                }.get(name)

            environment = {
                "PATH": launcher._path_separator().join(
                    (str(existing), str(managed))
                ),
                "PATHEXT": ".EXE;.CMD;.BAT",
                "PIPX_BIN_DIR": str(managed),
            }
            output = StringIO()
            with (
                mock.patch.dict(os.environ, environment, clear=False),
                mock.patch.object(launcher.shutil, "which", side_effect=which),
                mock.patch.object(
                    launcher.subprocess,
                    "run",
                    return_value=mock.Mock(returncode=0),
                ) as run,
                redirect_stdout(output),
            ):
                self.assertEqual(launcher._activate(dry_run=False), 0)

            run.assert_called_once_with(
                [str(root / pipx_name), "ensurepath", "--prepend"],
                check=False,
            )
            self.assertIn(str(existing / alias_name), output.getvalue())
            self.assertIn(
                f"zcf is the active clang-format: {managed / alias_name}",
                output.getvalue(),
            )

    def test_activate_dry_run_does_not_modify_path(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            zcf_name = launcher._application_name("zcf")
            alias_name = launcher._application_name("clang-format")
            pipx_name = launcher._application_name("pipx")
            for path in (root / zcf_name, root / alias_name, root / pipx_name):
                path.write_bytes(b"test")
                if not launcher._is_windows():
                    path.chmod(0o755)

            original_path = str(root)
            output = StringIO()
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "PATH": original_path,
                        "PATHEXT": ".EXE;.CMD;.BAT",
                        "PIPX_BIN_DIR": str(root),
                    },
                    clear=False,
                ),
                mock.patch.object(
                    launcher.shutil,
                    "which",
                    side_effect=lambda name: str(
                        root / launcher._application_name(name)
                    ),
                ),
                mock.patch.object(
                    launcher.subprocess,
                    "run",
                    return_value=mock.Mock(returncode=0),
                ) as run,
                redirect_stdout(output),
            ):
                self.assertEqual(launcher._activate(dry_run=True), 0)
                self.assertEqual(os.environ["PATH"], original_path)

            run.assert_called_once_with(
                [
                    str(root / pipx_name),
                    "ensurepath",
                    "--prepend",
                    "--dry-run",
                ],
                check=False,
            )

    def test_posix_command_search_requires_executable_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executable_dir = root / "executable"
            ignored_dir = root / "ignored"
            executable_dir.mkdir()
            ignored_dir.mkdir()
            executable = executable_dir / "clang-format"
            ignored = ignored_dir / "clang-format"
            executable.write_bytes(b"test")
            ignored.write_bytes(b"test")
            executable.chmod(0o755)
            ignored.chmod(0o644)

            original_directory = Path.cwd()
            try:
                os.chdir(root)
                relative_executable = Path("executable") / "clang-format"
                with (
                    mock.patch.dict(
                        os.environ,
                        {"PATH": "ignored:executable"},
                        clear=False,
                    ),
                    mock.patch.object(
                        launcher,
                        "_is_windows",
                        return_value=False,
                    ),
                    mock.patch.object(
                        launcher.os,
                        "access",
                        side_effect=lambda path, _: (
                            Path(path) == relative_executable
                        ),
                    ),
                ):
                    self.assertEqual(
                        launcher._command_paths("clang-format"),
                        [relative_executable],
                    )
            finally:
                os.chdir(original_directory)

    def test_posix_alias_exec_forwards_arguments_and_environment(self):
        binary = Path("/package/zcf/bin/zcf")
        with (
            mock.patch.object(launcher, "_is_windows", return_value=False),
            mock.patch.object(launcher, "_native_binary", return_value=binary),
            mock.patch.object(
                sys,
                "argv",
                ["clang-format", "-style=WFormat", "sample.cpp"],
            ),
            mock.patch.object(launcher.os, "execve") as execve,
        ):
            with self.assertRaisesRegex(
                AssertionError,
                "os.execve returned unexpectedly",
            ):
                launcher._run_native(clang_format_alias=True)

        forwarded = execve.call_args.args
        self.assertEqual(forwarded[0], binary)
        self.assertEqual(
            forwarded[1],
            ["clang-format", "-style=WFormat", "sample.cpp"],
        )
        self.assertEqual(
            forwarded[2]["ZCF_INVOKED_AS_CLANG_FORMAT"],
            "1",
        )

    def test_install_skill_argument_parser(self):
        repository, force = launcher._parse_install_skill_arguments(
            ["--force", "sample"]
        )
        self.assertEqual(repository, Path("sample"))
        self.assertTrue(force)

        with self.assertRaisesRegex(RuntimeError, "Usage"):
            launcher._parse_install_skill_arguments(["one", "two"])


if __name__ == "__main__":
    unittest.main()
