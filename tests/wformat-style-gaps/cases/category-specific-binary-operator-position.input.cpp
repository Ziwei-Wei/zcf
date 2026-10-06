int
evaluate(
    int first,
    int second,
    int third,
    bool ready,
    bool valid
    )
{
    int arithmetic = first // Keep arithmetic wrapped.
        + second;
    bool comparison = first // Keep comparison wrapped.
        < second;
    int shifted = first // Keep shift wrapped.
        << second;
    bool logical = ready && // Keep Boolean operators trailing.
        valid ||
        (third > 0);
    int selected = ready ? // Keep conditional operators trailing.
        arithmetic :
        shifted;
    selected = // Keep assignment trailing.
        comparison ? selected : third;

    return selected;
}
