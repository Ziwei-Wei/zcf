template <typename T, int Size>
struct Buffer
{
    T data[Size];

    template <int Index>
    constexpr decltype(auto)
    get()
    & requires(Index < Size)
    {
        return (data[Index]);
    }
};