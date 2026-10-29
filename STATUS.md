# zcf implementation status

Last updated: 2026-10-08

## Complete

- `zcf` is a single clang-format-compatible native executable.
- The reviewed C++ policy provides 22 independently configurable extensions.
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
  lambda bodies in call chains as aggregate levels;
- spacing options that also changed line breaks. `SpaceParameterPackEllipses`,
  `SpaceAnnotationsAndFunctionPointers`, and
  `SpaceAfterParenthesizedSpecifiers` now change spacing only. Function-pointer
  and `explicit(bool)` parameter breaking moved to
  `ForceMultilineFunctionSignatures`. Three narrow options cover the rest:
  `BreakPackExpansionInheritance`, `SeparateDeletedFunctions`, and
  `WrapPackIndexedFunctionBraces`. The last one restores function-brace
  wrapping that LLVM 20 loses when it misparses a C++26 pack-indexed trailing
  return type as a lambda. A brace rule that previously broke every brace on a
  line containing pack indexing was removed.

Each fix has a same-ID regression contract run by
`tests/validate-complete-format.ps1`.

Known upstream limitation: LLVM 20 can also join the declaration following a
pack-indexed function onto its closing-brace line. That parser defect is
outside the extension layer.

Post-format infrastructure was consolidated: one shared source scan and one
replacement set (`PostFormatSource`) serve both the structural and boundary
rules. Duplicated layout helpers were merged, which also fixed
`ForceMultilineFunctionSignatures` leaving multiple parameters bin-packed on one
line outside the preset (where `BinPackParameters: OnePerLine` had masked it).
The 22 extension options are declared once in `ExtensionOptions.def`.

`DeindentQualifiedFunctionNames` now changes only qualified-name indentation.
Parameter breaking for declarations whose qualified name is split by comments
(which upstream does not recognize as declarations) belongs to
`ForceMultilineFunctionSignatures`, which also applies the style's
`BreakAfterReturnType` to them as upstream does for recognized declarations.
It also keeps the `=` of `= delete`, `= default`, and pure-virtual `= 0` on the
parameter list's closing-parenthesis line; with `BreakBeforeBinaryOperators`
upstream otherwise breaks before that `=` whenever the return type is broken.

`BreakRefQualifierRequires`, `ContextSensitiveBracedInitializers`, and the
three spacing options (`SpaceParameterPackEllipses`,
`SpaceAnnotationsAndFunctionPointers`, `SpaceAfterParenthesizedSpecifiers`)
were removed from the policy. Their constructs now follow upstream spacing and
layout, and the bridge patch no longer needs a space-required hook.
`SeparateDeletedFunctions` was removed as well: `SeparateFunctionDeclarations`
separates `= delete` declarations on both sides, which produced identical output
on every contract and on the canonical fixture. `BlankLineBeforeReturn` and
`SeparateSwitchCaseBlocks` were removed too; blank lines before return
statements and between switch case blocks are left to the author.

The blank-line separation rules (`BlankLinesAroundControlStatements`,
`BlankLinesAroundLabels`, `SeparateDefinitionBlocksFromAll`,
`SeparateFunctionDeclarations`, and
`SeparatePreprocessorConditionalBlocks`) produce exactly one blank line,
collapsing extras even when `MaxEmptyLinesToKeep` allows more. They
rewrite only whitespace between lines, so adjacent comments are preserved.

Three independent blank-line rules cover what upstream
`SeparateDefinitionBlocks` deliberately skips:

- `SeparateDefinitionBlocksFromAll` separates every function, type, and
  namespace definition from every neighbor, including `#if`, `#elif`, `#else`,
  and `#endif` lines, at file, namespace, class, and `extern "C"` scope, and
  every multi-line lambda definition (`auto f = [...](...) { ... };`) at any
  scope. Lambdas passed as call arguments and single-line lambdas are not
  separated.
- `SeparateFunctionDeclarations` separates function declarations without a
  body (`int f();`, member functions, constructors, and operators) the same
  way. Variables, function pointers, and declarations inside function bodies
  are not separated.
- `SeparatePreprocessorConditionalBlocks` separates every balanced
  `#if`...`#endif` group, including nested groups, from its neighbors,
  regardless of what the group contains.

Leading comments, decorating macro lines directly above a definition, and a
preceding access specifier stay attached. Separation runs before LibFormat, as
upstream `SeparateDefinitionBlocks` does, so comment and macro alignment sees
the separated blocks; a post-format pass repeats it for lambdas that only
become multi-line while formatting. Statement boundaries come from raw tokens,
because LLVM 20 can merge a pack-indexed function with the following
declaration across `#endif`/`#if`.

The ZCF preset sets `BraceWrapping.BeforeLambdaBody: true`, so a multi-line
lambda body starts with its brace on its own line.

`BodyDrivenLambdaExpansion` expands only the body of a multi-statement or
overflowing lambda when its header fits; parameters are exploded one per line,
with the brace on its own line, only when the header overflows or the author
already exploded them.

### Option decoupling audit

Every option pair was formatted together and separately over all contract
inputs and the canonical fixture; the combined output must equal a clean merge
of the two single-option outputs. Options that build on one another, such as
`ArgumentIndentedClosingParentheses` indenting the closing parenthesis of a
list that `ForceMultilineFunctionSignatures` broke, compose. The audit found
and fixed these couplings:

- `BreakRequiresExpressionBraces` set every broken token of a declaration with
  a requires clause to the line's first column, so signature parameters lost
  their indentation and `ArgumentIndentedClosingParentheses` was overridden.
  It now affects only requires-expression braces.
- `ExpandNestedAggregateBraces` treated constructor braces (`T{...}`) and
  lambda closing braces as aggregate levels and could not render designated
  initializers, which left garbled indentation and froze calls inside the
  aggregate in the author's layout. It now expands only aggregate levels,
  renders `.name = value` elements one per line (`.name = {1, 2}` stays
  inline), keeps calls in LibFormat's layout, and wraps positional elements at
  the column limit.
- `ProgressiveArithmeticExpansion` treated any author line break in an
  expression, including breaks between call arguments, as a multi-line
  arithmetic layout. Only breaks around arithmetic operators count now. It
  also always keeps a two-operand expression that fits on one line; the former
  `CompactTwoOperandExpressions`, which did nothing without it, was merged in.
- `AlignMacrosAcrossDirectives` ended every alignment run at a blank or
  comment-only line. It now follows `AlignConsecutiveMacros.AcrossEmptyLines`
  and `AcrossComments`, so the blank lines inserted by
  `SeparatePreprocessorConditionalBlocks` are under the user's control.
- `CompactSingleArgumentCalls` was removed: it only acted when
  `ColumnLimit: 0` kept author line breaks and never changed preset output.

LLVM 20 (and LLVM main) decides whether a trailing comment after `}` ends a
scope from the brace's original line break, so a short block it merges onto
one line keeps its comment out of the alignment run until a second run. When
extensions are active and the formatted output contains such a merged block,
zcf repeats LibFormat and the post-format pass once, after the extension blank
lines exist, so one run is stable. Upstream styles without
extensions keep upstream behavior.

A follow-up audit fixed the remaining issues:

- Operator functions (`operator=(`, `operator()(`, `operator==(`) are
  signatures for `ForceMultilineFunctionSignatures`,
  `ArgumentIndentedClosingParentheses`, and the `= delete` attachment rule.
  Explicit member calls such as `value.operator=(other)` are not.
- The nested-aggregate renderer keeps trailing commas (and then lays the list
  out one element per line, as LibFormat does) and recognizes template angle
  brackets by LibFormat's attached spacing instead of guessing from a
  preceding name. When an aggregate holds comments or multi-line lambdas and
  falls back to LibFormat layout, elements take their indentation from the
  enclosing aggregate level, designated elements stay one per line, and
  innermost scalar lists stay on one line.
- The second formatting pass for merged short blocks runs only when a
  neighboring line also has a trailing comment, the only case where alignment
  differs.

Known upstream LLVM 20 behavior that zcf does not change:

- `a < b, c > d` inside a braced list is parsed as a template argument list.
- With `BraceWrapping.BeforeLambdaBody`, a lambda in a long member-call chain
  or as a designated initializer value puts its body brace on its own line
  under the lambda introducer.
- Trailing comments on nested aggregate elements can be indented or aligned
  inconsistently.

All code-review follow-ups are complete.

## Remaining release work

1. Register the repository's `pypi` environment as a PyPI trusted publisher.
2. Push tag `v0.1.0`.
3. Confirm public installation:

   ```text
   pipx install zcf
   zcf activate
   zcf status
   ```
