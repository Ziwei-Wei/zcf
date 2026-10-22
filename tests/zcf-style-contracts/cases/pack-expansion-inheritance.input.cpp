template <typename... Callables>
struct Overload : Callables... {
  using Callables::operator()...;
};
struct Plain : Base, Other {};
