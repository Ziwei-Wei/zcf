void
run()
{
    int value = 0;
    asm volatile ("" : "+r" (value));
}