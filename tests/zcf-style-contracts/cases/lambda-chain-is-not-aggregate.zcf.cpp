void
chain()
{
    auto mapped = source.and_then([](int value) { return Result {value + 1}; }).transform([](int value) { return value * 2; }).or_else([] { return Result {0}; });
}
