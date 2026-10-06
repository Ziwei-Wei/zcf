enum class Mode
{
    Alpha,
    Beta,
    Other,
};

void
work();

void
run(
    Mode mode
    )
{
    switch (mode)
    {
        case Mode::Alpha:
        {
            work();
            break;
        }
        case Mode::Beta:
        {
            work();
            break;
        }
        case Mode::Other:
        default:
        {
            break;
        }
    }
}
