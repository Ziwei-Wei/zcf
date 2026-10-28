template <typename T>
decltype(auto) Unwrap(T&& value) noexcept(noexcept(value.size()))
requires requires { typename std::remove_reference_t<T>::value_type; }
{
  return (value);
}
template <typename T>
concept IntegralLike = std::integral<T> && requires(T value) { typename std::make_unsigned_t<T>; };
