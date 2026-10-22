void fill() {
  Item items[] =
  {
    {
      {1, 2},
      {3, 4}
    },
    {
      {5, 6},
      {7, 8}
    }
  };
}

void fillWhen(bool enabled) {
  if (enabled) {
    Item items[] =
    {
      {
        {1, 2},
        {3, 4}
      },
      {
        {5, 6},
        {7, 8}
      }
    };
  }
}
