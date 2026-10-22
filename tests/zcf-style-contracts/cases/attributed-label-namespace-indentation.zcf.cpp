namespace sample
{
int
run()
{
    goto done;
    [[maybe_unused]] done:

    return 1;
}

struct Worker
{
    void
    step()
    {
        {
            goto next;
            [[likely]] next:
            work();
        }
    }
};
} // namespace sample
