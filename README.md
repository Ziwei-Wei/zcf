# zcf

**zcf** means **z-clang-format**. It is a clang-format-compatible native
formatter built from LLVM 20.1.8 with a reviewed custom C++ policy.

zcf extends the original clang-format rather than replacing its configuration
model. Existing `.clang-format` and `_clang-format` files, predefined styles,
command-line options, editor integrations, and supported languages remain
compatible. Projects can adopt zcf without rewriting their clang-format
configuration; zcf-specific options are additive and upstream behavior is
preserved when those options are not enabled.

The build produces one formatter under two names:

```text
zcf                 user-facing command
clang-format        byte-identical alias for IDE discovery
```

The custom policy is enabled only for C++. Every upstream style and non-C++
language retains upstream behavior unless a configuration explicitly enables
an extension.

## Install with pipx

Once zcf is published to PyPI:

```text
pipx install zcf
zcf activate
```

If pipx reports that its application directory is not yet on `PATH`, use:

```text
pipx run zcf activate
```

The wheel exposes both `zcf` and `clang-format`. `zcf activate`:

1. lists every `clang-format` currently visible on `PATH`;
2. asks pipx to prepend its managed application directory;
3. verifies that pipx's zcf-backed `clang-format` is active; and
4. never deletes or overwrites another package manager's files.

Preview or diagnose without changing `PATH`:

```text
zcf activate --dry-run
zcf status
```

Restart running terminals and IDEs after activation.

Install the zcf agent skill into a repository so compatible coding agents can
discover and use the formatter automatically:

```text
cd path/to/repository
zcf install-skill
git add .github/skills/zcf/SKILL.md
```

The command is idempotent and refuses to overwrite an existing skill unless
`--force` is specified. A different repository can be provided explicitly:

```text
zcf install-skill path/to/repository
```

Each wheel contains one platform-native zcf binary. Separate wheels are needed
for each supported operating system and architecture, but they are independent
of the Python runtime version (`py3-none-<platform>`).

The wheel workflow covers:

- Windows x64 and ARM64;
- macOS Intel and ARM64; and
- manylinux 2.28 x64 and ARM64.

Tags matching `v<project-version>` publish all validated wheels through PyPI
trusted publishing. The repository's `pypi` GitHub environment must be
registered as a trusted publisher before the first release.

## Use zcf

Command-line usage is the same as clang-format:

```text
zcf -style=LLVM file.cpp
zcf -style=file -i file.cpp
clang-format -style=file file.cpp
```

The reviewed C++ policy uses `ColumnLimit: 140`, leading binary operators, no
consecutive assignment alignment, right-aligned scalar matrices, progressive
call and arithmetic expansion, and 26 independently configurable extensions.

## Build from source

Initialize the pinned LLVM submodule:

```text
git submodule update --init --recursive
git -C third_party/llvm-project rev-parse HEAD
```

Expected LLVM commit:

```text
87f0227cb60147a26a1eeb4fb06e3b505e9c7261
```

Configure and build with a single-config generator:

```text
cmake --preset release
cmake --build --preset zcf
```

Output:

```text
build/release/bin/zcf
build/release/bin/clang-format
```

On Windows the files use the `.exe` suffix. A regular PowerShell prompt should
run the build inside the Visual Studio developer environment so Windows SDK
tools are available:

```powershell
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat'
cmd /c "call `"$vcvars`" >nul && pwsh -File scripts\build.ps1 -Jobs 4"
```

To reuse an existing static LLVM build:

```powershell
.\scripts\prepare-llvm-static.ps1 -Jobs 8

cmake -S . -B build\release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DLLVM_PROJECT_SOURCE_DIR=third_party\llvm-project `
  -DCUSTOM_LLVM_DIR=build\llvm-static\lib\cmake\llvm `
  -DCUSTOM_LLVM_TABLEGEN=build\llvm-static\bin\llvm-tblgen.exe

cmake --build build\release --target zcf
```

The LLVM libraries and generated tables must come from the same pinned commit as
the Clang sources.

## Build a wheel

Build zcf first, then package that platform's native executable:

```powershell
py -m pip install build "scikit-build-core>=1.1,<2" pipx
py -m build --wheel `
  -C "cmake.args=-DZCF_EXECUTABLE=$PWD\build\release\bin\zcf.exe"
```

Validate the resulting wheel with an isolated pipx installation:

```powershell
py tests\validate_pipx_package.py `
  --wheel .\dist\zcf-0.1.0-py3-none-win_amd64.whl
```

The test verifies both entry points, version reporting, custom-policy formatting,
conflict discovery, dry-run activation, and uninstall cleanup.

## Architecture

Upstream LibFormat remains pristine:

```text
third_party/llvm-project/       pinned upstream submodule
integration/patches/            minimal policy-free bridge
integration/clang-format/       private LibFormat adapter
extensions/clang-format/        custom C++ policy and options
build/llvm-overlay/             ignored generated patched worktree
clang-format/ClangFormat.cpp    zcf command-line tool
```

`scripts/prepare_llvm_overlay.py` verifies the upstream worktree, creates the
overlay at the pinned commit, applies the bridge, and rejects unexpected overlay
edits. Formatting policy remains in the separately compiled extension modules.

The extension context is scoped per formatting call. With every extension
disabled—or outside C++—the bridge remains behaviorally identical to upstream.

## Validation

Build and smoke tests:

```text
cmake --build build/release --target zcf
cmake --build build/release --target smoke-zcf
```

Custom-policy contract tests:

```text
cmake --build build/release --target validate-p0-format
cmake --build build/release --target validate-p1-format
cmake --build build/release --target validate-p2-format
```

Upstream parity:

```text
cmake --build build/release --target compare-format-gtest
```

The parity suite covers 210 formatting, style-dump, round-trip, language, and CLI
cases. The most recent validated state passes all 210.

Python launcher and wheel tests:

```text
py -m unittest tests/test_zcf_launcher.py -v
python tests/validate_pipx_package.py --wheel <wheel>
```

## Static-link behavior

The LLVM and Clang child builds disable shared LLVM/Clang libraries and optional
compression/XML dependencies. `-StaticMsvcRuntime` additionally selects `/MT`
for Windows builds.

Verify runtime dependencies:

```powershell
dumpbin /dependents build\release\bin\zcf.exe
```

```bash
ldd build/release/bin/zcf       # Linux
otool -L build/release/bin/zcf  # macOS
```

LLVM or Clang shared libraries should not appear. Normal platform runtime
libraries may remain unless the selected toolchain supports a fully static
runtime.
