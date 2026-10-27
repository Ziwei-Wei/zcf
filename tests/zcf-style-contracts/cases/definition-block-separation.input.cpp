template <typename... Values> void consume(Values&&... values) { (void)sizeof...(values); }
#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L
template <unsigned Index, typename... Args> auto select(Args... args) -> Args...[Index] { return args...[Index]; }
#endif
#if defined(__cpp_variadic_friend) && __cpp_variadic_friend >= 202403L
template <typename... TrustedTypes> struct Vault { friend TrustedTypes...; };
#endif
int declared();
struct Point { int x; int y; };
int counter = 0;
#ifdef ZCF_TRACE
int trace() { return 1; }
#else
int trace() { return 0; }
#endif
