void configure(int a)
{
    Mixed nested = {.table = std::map<int, std::vector<int>>{}, .shift = a >> 2, .layers = {{.pick = 1, .leaf = {1, 2}}}};
    Mixed commented = {.text = std::string(3, 'x'), // first element
                       .layers = {{.pick = 1, .leaf = {1, 2}}}};
    Mixed trailing = {.text = 1, .layers = {{.pick = 1, .leaf = {1, 2},},},};
}
