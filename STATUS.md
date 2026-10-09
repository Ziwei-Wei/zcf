# zcf implementation status

Last updated: 2026-10-08

## Complete

- `zcf` is a single clang-format-compatible native executable.
- The reviewed C++ policy provides 26 independently configurable extensions.
- Upstream LLVM 20.1.8 remains pinned and pristine behind a generated overlay.
- `pipx install zcf` packages the native executable and exposes:
  - `zcf`
  - `clang-format`
- `zcf activate`, `zcf activate --dry-run`, and `zcf status` safely manage PATH
  precedence without deleting another formatter.
- `zcf install-skill` installs the packaged project skill under
  `.github/skills/zcf/SKILL.md` with idempotent and conflict-safe behavior.
- Windows x64 and ARM64 wheel jobs pass end to end.
- Local Windows validation passes:
  - signature, structure, and complete-policy suites;
  - isolated pipx wheel installation and uninstall;
  - activation/conflict detection;
  - 210/210 upstream parity tests.
- Cross-platform overlay generation now uses Python rather than PowerShell.

## GitHub Actions progress

Workflow:
[Cross-platform wheels](.github/workflows/wheels.yml)

### Initial run

[Run 37394782489](https://github.com/Ziwei-Wei/zcf/actions/runs/37394782489)

- Windows x64: passed.
- Windows ARM64: passed.
- Linux x64/ARM64: reached final link; failed because local zcf objects used
  RTTI while LLVM was built without RTTI.
- macOS Intel/ARM64: local project configuration required the C language for
  LLVM's LibEdit probe.

Fix:

- enabled C and C++ in the local zcf project;
- matched LLVM's no-RTTI/no-exceptions defaults on non-MSVC toolchains.

### Portability rerun

[Run 37401260031](https://github.com/Ziwei-Wei/zcf/actions/runs/37401260031)

- Windows x64: passed.
- Windows ARM64: passed.
- Linux x64/ARM64: native formatter build and smoke test passed; wheel staging
  used the host workspace path instead of the container workspace path.
- macOS Intel/ARM64: native formatter and wheel build passed; POSIX unit-test
  fixtures were not marked executable.

Current fix:

- use `$GITHUB_WORKSPACE` inside manylinux containers;
- mark fake POSIX entry points executable in launcher tests.

### Green PR matrix

[Run 37406161535](https://github.com/Ziwei-Wei/zcf/actions/runs/37406161535)

- Windows x64: passed.
- Windows ARM64: passed.
- macOS Intel: passed.
- macOS ARM64: passed.
- manylinux 2.28 x64: passed.
- manylinux 2.28 ARM64: passed.
- Every wheel completed native formatting, Python launcher, real pipx
  installation, `zcf activate`, custom-policy formatting, uninstall, and
  artifact-upload tests.

## Extension conflict fixes

Code review against upstream clang-format behavior found and fixed:

- attributed labels indented by raw brace depth, which counted namespace braces
  and ignored `UseTab`;
- qualified-name continuation dedenting template and parameter arguments to
  column 0;
- call compaction joining adjacent string literals despite
  `BreakAdjacentStringLiterals`;
- `BlankLineBeforeReturn` separating a return from a braceless control header,
  a switch label, or an opening brace, and splitting it from its comment;
- `BlankLinesAroundControlStatements` deleting trailing comments;
- nested-aggregate expansion hardcoding four-space indentation, ignoring
  `UseTab`, merging an expanded aggregate into a short function, and treating
  lambda bodies in call chains as aggregate levels.

Each fix has a same-ID regression contract run by
`tests/validate-complete-format.ps1`.

Remaining review follow-ups:

1. Rename or split options whose names describe spacing but also change line
   breaks (`SpaceParameterPackEllipses`,
   `SpaceAnnotationsAndFunctionPointers`, `SpaceAfterParenthesizedSpecifiers`).
2. Share one source scan between the structural and boundary post-format
   passes and consolidate duplicated layout helpers.

## Remaining release work

1. Register the repository's `pypi` environment as a PyPI trusted publisher.
2. Push tag `v0.1.0`.
3. Confirm public installation:

   ```text
   pipx install zcf
   zcf activate
   zcf status
   ```
