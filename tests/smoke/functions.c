/* Smoke test: function calls and recursion */

extern int printf(const char *fmt, ...);

int add(int a, int b)
{
    return a + b;
}

int factorial(int n)
{
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

int many_args(int a, int b, int c, int d, int e)
{
    return a + b + c + d + e;
}

int call_printf(void)
{
    return printf("hello %s %d\n", "world", 42);
}
