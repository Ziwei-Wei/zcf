int
declaration(
    int left,
    int right
);

int
definition(
    int left,
    int right
)
{
    return left + right;
}

int
use()
{
    return declaration(
        1,
        2
    );
}
