template <unsigned Index, typename... Args>
auto select(Args... args) -> Args... [Index]
{
  return args...[Index];
}
