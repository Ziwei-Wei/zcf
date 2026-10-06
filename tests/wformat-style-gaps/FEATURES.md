# WFormat feature catalog

WFormat on LLVM 20.1.8 provides **26 independently configurable C++ extension
options**:

- 4 P0 core behaviors;
- 9 P1 fidelity behaviors;
- 6 P2 specialized behaviors; and
- 7 reviewed preset rules.

Every option is disabled by default for upstream styles. `BasedOnStyle: WFormat`
enables the complete reviewed policy. Non-C++ languages fall back to upstream
LLVM behavior with the extension layer disabled.

The machine-readable source of truth, including audit evidence, risks,
intentional exclusions, and oracle defects, is
[manifest.json](manifest.json).

## P0

| Option | Behavior | Contract |
| --- | --- | --- |
| `ArgumentIndentedClosingParentheses` | Indent the closing `)` of an already-wrapped definition, declaration, or call to the item column. | [input](cases/closing-parenthesis-argument-indent.input.cpp) / [target](cases/closing-parenthesis-argument-indent.wformat.cpp) |
| `ForceMultilineFunctionSignatures` | Give non-empty declarations and definitions the reviewed multiline signature shape. Calls are not forced multiline. | [input](cases/force-multiline-function-signatures.input.cpp) / [target](cases/force-multiline-function-signatures.wformat.cpp) |
| `ContextSensitiveBracedInitializers` | Distinguish constructor members, named objects, temporaries, and return braced lists. | [input](cases/context-sensitive-braced-initializers.input.cpp) / [target](cases/context-sensitive-braced-initializers.wformat.cpp) |
| `BlankLineBeforeReturn` | Insert a blank line before `return` unless it immediately follows an opening brace. | [input](cases/blank-line-before-return.input.cpp) / [target](cases/blank-line-before-return.wformat.cpp) |

## P1

| Option | Behavior | Contract |
| --- | --- | --- |
| `BreakConstructorDestructorSpecifiers` | Break selected constructor/destructor declarations after `constexpr`, `consteval`, or `virtual`. | [input](cases/break-constructor-destructor-specifiers.input.cpp) / [target](cases/break-constructor-destructor-specifiers.wformat.cpp) |
| `CompactSingleArgumentCalls` | Recompact a safe call containing one simple expression argument. | [input](cases/single-argument-call-compaction.input.cpp) / [target](cases/single-argument-call-compaction.wformat.cpp) |
| `ExpandNestedAggregateBraces` | Render deep aggregates as structural brace scopes while retaining compact scalar leaf rows. Native right alignment still owns ordinary two-level matrices. | [input](cases/nested-aggregate-brace-expansion.input.cpp) / [reviewed target](cases/nested-aggregate-brace-expansion.reviewed.cpp) |
| `BreakRequiresExpressionBraces` | Put requires-expression braces into the reviewed multiline layout. | [input](cases/requires-expression-brace-layout.input.cpp) / [target](cases/requires-expression-brace-layout.wformat.cpp) |
| `SpaceParameterPackEllipses` | Apply reviewed spacing to template, function, using-declaration, and pack-indexing ellipses. | [input](cases/parameter-pack-ellipsis-spacing.input.cpp) / [target](cases/parameter-pack-ellipsis-spacing.wformat.cpp) |
| `SpaceAnnotationsAndFunctionPointers` | Format SAL-style annotations and grouped function-pointer declarators independently from ordinary calls. | [input](cases/annotation-function-pointer-spacing.input.cpp) / [target](cases/annotation-function-pointer-spacing.wformat.cpp) |
| `NormalizeIntegerLiteralCase` | Lowercase base prefixes and uppercase hexadecimal digits and standard integer suffixes. | [input](cases/integer-literal-case-normalization.input.cpp) / [target](cases/integer-literal-case-normalization.wformat.cpp) |
| `BlankLinesAroundControlStatements` | Separate selected `if`, `for`, `while`, and `switch` blocks from adjacent statements. | [input](cases/blank-lines-around-control-statements.input.cpp) / [target](cases/blank-lines-around-control-statements.wformat.cpp) |
| `AlignMacrosAcrossDirectives` | Preserve object-like macro replacement-column alignment across conditional directives. | [input](cases/cross-directive-macro-alignment.input.cpp) / [target](cases/cross-directive-macro-alignment.wformat.cpp) |

## P2

| Option | Behavior | Contract |
| --- | --- | --- |
| `DeindentQualifiedFunctionNames` | Keep comment-separated qualified definition-name components at the function-name column without changing calls or assignments. | [input](cases/qualified-function-name-continuation-indent.input.cpp) / [target](cases/qualified-function-name-continuation-indent.wformat.cpp) |
| `BreakRefQualifierRequires` | Keep a function ref qualifier and its trailing `requires` clause together on the reviewed continuation line. | [input](cases/ref-qualifier-requires-layout.input.cpp) / [target](cases/ref-qualifier-requires-layout.wformat.cpp) |
| `SpaceAfterParenthesizedSpecifiers` | Format reviewed parenthesized declaration specifiers such as `explicit (condition)` and delete-with-reason. | [input](cases/space-after-explicit.input.cpp) / [target](cases/space-after-explicit.wformat.cpp) |
| `BlankLinesAroundLabels` | Insert blank lines around non-case labels. | [input](cases/blank-lines-around-labels.input.cpp) / [target](cases/blank-lines-around-labels.wformat.cpp) |
| `IndentAttributedLabels` | Indent an attributed label to its enclosing brace depth. | [input](cases/attributed-label-indentation.input.cpp) / [target](cases/attributed-label-indentation.wformat.cpp) |
| `SeparateClosingDirectiveComments` | Keep `#else` and `#endif` comments outside neighboring trailing-comment alignment groups. | [input](cases/preprocessor-closing-comment-alignment-boundary.input.cpp) / [target](cases/preprocessor-closing-comment-alignment-boundary.wformat.cpp) |

## Reviewed preset rules

| Option | Behavior | Contract |
| --- | --- | --- |
| `ProgressiveCallExpansion` | Keep fitting calls compact, expand the outer argument list first, and descend only while a nested line still overflows. | [input](../cpp-style-examples/wformat-preset-progressive-calls.input.cpp) / [target](../cpp-style-examples/wformat-preset-progressive-calls.expected.cpp) |
| `ProgressiveArithmeticExpansion` | Expand additive, multiplicative, grouping, and call layers progressively under the active `ColumnLimit`. Assignments break after `=`; overflowing returns keep the first operand after `return`. | [input](../cpp-style-examples/wformat-preset-progressive-arithmetic.input.cpp) / [target](../cpp-style-examples/wformat-preset-progressive-arithmetic.expected.cpp) |
| `CompactTwoOperandExpressions` | Recompact a fitting safe two-operand expression without collapsing an internal arithmetic layer. | [input](../cpp-style-examples/wformat-preset-compact-two-operands.input.cpp) / [target](../cpp-style-examples/wformat-preset-compact-two-operands.expected.cpp) |
| `BodyDrivenLambdaExpansion` | Expand a parameterized lambda when one physical body line has multiple statement terminators or the lambda itself overflows. Semicolons in a `for` header do not count. | [input](cases/body-driven-lambda-expansion.input.cpp) / [target](cases/body-driven-lambda-expansion.wformat.cpp) |
| `VerticalTernaryExpressions` | Fully verticalize a ternary only when its own range still exceeds the available continuation width. | [input](cases/vertical-ternary-expressions.input.cpp) / [target](cases/vertical-ternary-expressions.wformat.cpp) |
| `SeparateSwitchCaseBlocks` | Insert one blank line between completed braced case blocks while preserving fall-through labels. | [input](cases/switch-case-block-separation.input.cpp) / [target](cases/switch-case-block-separation.wformat.cpp) |
| `ScopeStyleNestedTemplates` | Expand an overflowing nested-template range and align every closing `>` with its matching scope. | [input](cases/scope-style-nested-templates.input.cpp) / [target](cases/scope-style-nested-templates.wformat.cpp) |

## Native preset policy

WFormat also selects upstream LLVM options rather than duplicating them:

```yaml
ColumnLimit: 140
AlignArrayOfStructures: Right
AlignConsecutiveAssignments:
  Enabled: false
BreakBeforeBinaryOperators: All
BreakBinaryOperations: RespectPrecedence
AlignOperands: DontAlign
PenaltyBreakAssignment: 1000
```

Native clang-format owns Allman braces, return-type breaks, list packing after a
natural wrap, inheritance and constructor-initializer colon spacing,
requires-clause indentation, ordinary trailing-comment alignment, and spaced
C++ arrows.

## Intentional exclusions

WFormat does not perform source-semantic rewrites. The excluded set includes:

- forced wrapping of fitting calls;
- inline-assembly-specific spacing;
- assignment-column alignment;
- category-specific binary-operator placement;
- global semantic continuation indentation;
- a space after lambda captures;
- legacy dynamic exception-specification breaks;
- Boolean parenthesis insertion;
- enum-comma, loop, or semicolon rewrites;
- comment injection or string-literal content changes; and
- final-newline stripping.

Known Uncrustify parser and rendering defects are recorded as oracle limitations,
not formatting requirements. See the `excludedBehaviors` and
`oracleLimitationsAndDefects` arrays in [manifest.json](manifest.json).

## Validation

Run the extension suites:

```text
cmake --build build/release --target validate-p0-format
cmake --build build/release --target validate-p1-format
cmake --build build/release --target validate-p2-format
```

Validate all 19 oracle-derived same-ID contracts:

```text
pwsh -File scripts/validate-wformat-style-gap-cases.ps1
```

Validate upstream behavior when extensions are disabled:

```text
cmake --build build/release --target compare-format-gtest
```

The canonical fixture remains interaction and syntax-coverage evidence:

[wformat_supported_style_details.cpp](../cpp-style-examples/wformat_supported_style_details.cpp)
