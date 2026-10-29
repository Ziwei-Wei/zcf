struct Value {
  Value &operator=(
      const Value &
      ) = delete;
  bool operator==(
      const Value &other
      ) const;
  int operator()(
      int left,
      int right
      );
  explicit operator bool() const;
};
void use(
    Value &a,
    Value &b
    ) {
  a.operator=(b);
  int r = a.operator()(1, 2);
}
