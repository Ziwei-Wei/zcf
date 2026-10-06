int
adjust(
    int value
    );

bool
check(
    int value,
    int bias
    ) noexcept;

auto
make_lambdas(
    int bias
    )
{
    const auto compact = [bias](int value) { return value + bias; };
    const auto for_header = [bias](int value) { for (;;) { return value + bias; } };
    const auto separate_lines = [bias](int value) {
        const int adjusted = adjust(value);
        return adjusted + bias;
    };
    const auto multiple_statements = [bias](int value) { const int adjusted = adjust(value); return adjusted + bias; };
    const auto noexcept_specifier = [bias](int value) noexcept(noexcept(check(value, bias))) { const int adjusted = adjust(value); return adjusted + bias; };
    const auto overflowing = [bias](int valueWithAnExtremelyLongDescriptiveNameThatPushesTheCompleteLambdaExpressionPastTheConfiguredColumnLimit) { return valueWithAnExtremelyLongDescriptiveNameThatPushesTheCompleteLambdaExpressionPastTheConfiguredColumnLimit + bias; };
    return multiple_statements;
}
