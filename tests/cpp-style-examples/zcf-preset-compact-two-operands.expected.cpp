long long
compact_delta()
{
    const auto delta = measuredValue - targetValue;
    const auto layered =
        sampleIndex
        * stride
        + channelOffset;

    return delta + layered;
}
