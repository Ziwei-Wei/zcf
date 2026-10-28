void
dispatch()
{
    double discriminant = std::fma(
        -4.0 * quadratic_a,
        quadratic_c,
        quadratic_b * quadratic_b);
    *callback_count = receiver.ready_count + ZCFDispatchMessage(
        1,
        2);
    auto [number_end, number_error] = std::to_chars(
        number_buffer,
        number_buffer + sizeof(number_buffer),
        folded);
}
