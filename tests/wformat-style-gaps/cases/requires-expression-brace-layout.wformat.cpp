#include <concepts>

template <typename T>
concept Addable = requires(T value)
{
    value + 1;
};