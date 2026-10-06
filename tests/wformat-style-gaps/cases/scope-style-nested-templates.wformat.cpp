#include <variant>

template <typename T>
struct Container
{};

struct ValueWithAnExtremelyLongDescriptiveNameThatForcesTheNestedTemplatePastTheConfiguredColumnLimit
{};

struct ErrorWithAnExtremelyLongDescriptiveNameThatForcesTheNestedTemplatePastTheConfiguredColumnLimit
{};

using CompactResult = std::variant<Container<int>, int>;
using ExpandedResult = std::variant<
    Container<
        ValueWithAnExtremelyLongDescriptiveNameThatForcesTheNestedTemplatePastTheConfiguredColumnLimit
        >,
    ErrorWithAnExtremelyLongDescriptiveNameThatForcesTheNestedTemplatePastTheConfiguredColumnLimit
    >;
using DeepExpandedResult = std::variant<
    Container<
        Container<
            ValueWithAnExtremelyLongDescriptiveNameThatForcesTheNestedTemplatePastTheConfiguredColumnLimit
            >
        >,
    ErrorWithAnExtremelyLongDescriptiveNameThatForcesTheNestedTemplatePastTheConfiguredColumnLimit
    >;
