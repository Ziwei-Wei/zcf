int Run(int bias) {
  int base = bias;
  auto twice = [bias](int value) { int doubled = value * 2; return doubled + bias; };
  auto same = [](int value) { return value; };
  if (base > 0) { auto shifted = [base](int value) { int moved = value + base; return moved; }; base = shifted(base); }
  return twice(same(base));
}
