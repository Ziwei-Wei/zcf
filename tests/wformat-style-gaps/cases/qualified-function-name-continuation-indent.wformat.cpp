namespace example
{

struct Widget
{
    static long
    compute(
        int value
        );
};

}

long
example:: // Namespace component.
Widget::  // Class component.
compute(
    int value
    )
{
    return value;
}