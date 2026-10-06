template <bool Strict>
struct Flag
{
    explicit (Strict) Flag(
        int value
        ):
        value(value)
    {
    }

    void
    reject() = delete ("reason");

    int value;
};