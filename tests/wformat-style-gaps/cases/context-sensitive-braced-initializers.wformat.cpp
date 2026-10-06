struct Point
{
    int x;
    int y;
};

struct Holder
{
    Holder(
        int value
        ):
        point_{value, value}
    {
    }

    Point point_;
};

Point
make()
{
    Point local {1, 2};
    auto temporary = Point {3, 4};

    return {5, 6};
}