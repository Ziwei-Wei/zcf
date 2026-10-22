struct Handle {
  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;
  void reject() = delete("reason");
  int value;
};
