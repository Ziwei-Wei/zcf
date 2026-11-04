// A line-leading all-caps macro call becomes its own statement once a break
// follows it, so its parentheses are not a signature, but a declaration that
// follows it is. All-caps constructors are still signatures.
struct Node {
  ALIGNAS_TYPE(Value) char buffer[sizeof(Value)];
  NODE(int value);
  NODE(const NODE &other) = delete;
  EXPORT_API(int) int exported(int value);
};
