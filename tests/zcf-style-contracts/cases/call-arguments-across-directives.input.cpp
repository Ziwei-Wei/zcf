void
report(char* buffer, const char* file, int line)
{
#ifdef PLATFORM_WINDOWS
    sprintf_s(
#else
    sprintf(
#endif
        buffer, "Assert in %s(%d)\n", file, line);

    length = FindBestMatch(context, index, &offset
#ifdef DETAILED_STATS
        , stats
#endif
        );

    total = first
#ifdef EXTRA_TERM
        + second
#endif
        ;
}
