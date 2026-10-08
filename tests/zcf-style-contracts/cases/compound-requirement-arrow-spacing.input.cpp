#include <concepts>

template <typename T>
struct Box
{
    Box(T value);
};

Box(const char*) -> Box<const char*>;

template <typename T>
concept Converts = requires(T value) { { value } -> std::same_as<T>; };

auto transform() -> int;
