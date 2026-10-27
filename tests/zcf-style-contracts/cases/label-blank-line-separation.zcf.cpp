int
run(
    bool jump
    )
{
    prepare();

retry:

    if (jump) { goto retry; }

done:

    return 1;
}
