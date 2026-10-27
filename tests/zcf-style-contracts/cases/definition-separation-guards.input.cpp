namespace guards {
int first;
// Leading comments stay attached to their definition.
struct Leading { int value; };
/*
 * Block comments stay attached as well.
 */
struct Documented { int value; };
class Members {
public:
  Members() {}
  int value;
private:
  int Hidden() const { return value; }
};
enum class Mode { Fast, Safe };
int TryBlock() try { return 0; } catch (...) { return 1; }
template <typename T> int Requires(T value) requires requires { value + 1; } { return 1; }
int last;
}
extern "C" {
int c_first;
void CFunction() {}
}
void Block(int x) {
  int before = x;
  auto twice = [](int v) { int w = v; return w * 2; };
  auto once = [](int v) { return v; };
  Call([](int v) { int w = v; return w; });
  if (x) { auto inner = [](int v) { int w = v; return w; }; inner(x); }
}
