#define VALUE_A 1       // first value
#if defined(ZCF_BRANCH) // selected branch
#define VALUE_B 2       // selected value
#else // fallback branch
#define VALUE_C 3       // fallback value
#endif // ZCF_BRANCH
