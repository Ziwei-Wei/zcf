struct Shape {
  void reject() = delete("reason");
  void plain() = delete;
  virtual double area() const = 0;
  virtual void scale(double factor) = 0;
  auto convert() const -> int = delete;
  int value = 0;
};
void free_function() = delete;
int counter = 0;
