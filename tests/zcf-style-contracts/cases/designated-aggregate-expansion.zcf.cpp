void configure(int base, int offset) {
  Mixed value =
  {
    .text = std::string(3, 'x'),
    .variant = std::variant<int, std::string>{std::in_place_index<0>, offset},
    .layers =
    {
      {
        .pick = std::max<int>(base, offset),
        .twice = [base] { return base * 2; }(),
        .leaf = {1, 2}
      }
    }
  };
}
