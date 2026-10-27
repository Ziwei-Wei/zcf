int counter;

int
first();

int
second(
    int value
    );

using Callback = void(int);

// Creates the widget.
Widget*
create();

struct Widget
{
    Widget();

    int value;

    int
    read() const;

    void
    write(
        int next
        );

    int (*hook)(
        int
        );

    friend bool
    operator==(const Widget&, const Widget&);
};

void
run()
{
    int local();
    int x = 0;
}
