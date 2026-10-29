void
update(
    int value
    )
{
    prepare();              // before the merged block
    if (value) { apply(); } // after the merged block
    finish();               /* following statement */
}
