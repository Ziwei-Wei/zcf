enum class Mode
{
    First,
    Second
};

bool
choose(
    bool first,
    bool second,
    bool third
    )
{
    if (first && second || third)
    {
        return first && second;
    }
    else
    {
        return false;
    }
}

bool
normalize(
    bool first,
    int second,
    int third
    )
{
    bool result = first && (second > third);

    return first && (second > third);
}

void
spin()
{
    while (true)
    {
    }
}