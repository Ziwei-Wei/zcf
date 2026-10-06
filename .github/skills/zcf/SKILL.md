---
name: zcf
description: Format or check clang-format-compatible source files with zcf. Use when asked to format code, apply clang-format, fix formatting, or validate repository formatting.
license: Apache-2.0 WITH LLVM-exception
---

# zcf

Use `zcf` as the repository's clang-format-compatible formatter.

## Workflow

1. Confirm that `zcf` is available:

   ```text
   zcf --version
   ```

   If it is missing, ask before installing it:

   ```text
   pipx install zcf
   ```

2. Locate the nearest `.clang-format` or `_clang-format` file. Reuse it with
   `-style=file`; do not invent or replace repository formatting policy.

3. Inspect the working tree before formatting. Preserve unrelated user changes
   and do not format an entire repository unless the user explicitly requests
   it.

4. Format only the requested or changed source files:

   ```text
   zcf -style=file -i path/to/file.cpp
   ```

   For a narrow edit in a file with unrelated changes, prefer one or more
   explicit line ranges:

   ```text
   zcf -style=file -i -lines=START:END path/to/file.cpp
   ```

5. Verify the result:

   ```text
   zcf -style=file --dry-run -Werror path/to/file.cpp
   git diff --check
   ```

6. Review the diff and report the formatted files and validation result.

## Safety

- Do not silently fall back to a different formatter.
- Do not overwrite repository configuration.
- Do not discard pre-existing changes.
- Surface formatter errors instead of treating partial output as success.
