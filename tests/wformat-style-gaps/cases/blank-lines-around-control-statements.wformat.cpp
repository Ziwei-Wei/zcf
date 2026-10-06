void
run()
{
    prepare();

    if (ready)
    {
        work();
    }

    finish();

    for (int index = 0; index < 1; ++index)
    {
        work();
    }

    finish();

    while (ready)
    {
        work();
        break;
    }

    finish();

    switch (value)
    {
        case 0:
            break;
    }

    finish();
}