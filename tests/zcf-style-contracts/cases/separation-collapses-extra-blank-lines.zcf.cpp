int
run(
    int value
    )
{
    prepare();

    if (value) { work(); }

    finish();



    keep();

done:

    cleanup();

    return value;
}

struct Handle
{
    Handle(
        const Handle&
        ) = delete;

    int value;
};

void
pick(
    int mode
    )
{
    switch (mode)
    {
        case 1:
        {
            work();
            break;
        }

        default: break;
    }
}

struct Record
{

public: // trailing comment after an access specifier

    Record(
        const Record&
        ) = delete; // why deleted

    /* leading */ int value;
};

int
runWithComments(
    int mode
    )
{
    prepare(); // before control

    /* lead */ if (mode)
    {
        work();
    } // after control

    finish();

done: // label comment

    /* lead */ cleanup();

    switch (mode)
    {
        case 1:
        {
            break;
        } // end case

        /* lead */ default:
            break;
    }

    // explains
    return mode;
}
