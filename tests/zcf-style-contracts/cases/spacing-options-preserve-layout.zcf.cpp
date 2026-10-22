template <typename ... Callables> struct Overload : Callables... {
  using Callables::operator() ...;
};
template <typename ... Ts> auto first() { return std::array{Ts ...[0]}; }
typedef Result (ZCF_STDCALL *Callback)(_In_reads_ (size) const void *data,
                                       int size);
void (*plain)(int, int) = nullptr;
template <bool Strict> struct Flag {
  explicit (Strict) Flag(int value) : value(value) {}
  void reject() = delete ("reason");
  int value;
};
