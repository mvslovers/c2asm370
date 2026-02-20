/* Smoke test: basic data types and arithmetic */
int test_types(void)
{
    char c = 'A';
    short s = 1234;
    int i = 56789;
    long l = -100000L;
    unsigned int u = 0xDEADBEEF;

    int sum = (int)c + s + i + (int)l + (int)u;

    return sum;
}
