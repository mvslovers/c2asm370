/* Smoke test: structs, unions, arrays, pointers */

struct point {
    int x;
    int y;
};

union value {
    int i;
    char c[4];
};

int struct_access(struct point *p)
{
    return p->x + p->y;
}

void struct_assign(struct point *dst, struct point *src)
{
    *dst = *src;
}

int array_sum(int *arr, int n)
{
    int sum = 0;
    int i;
    for (i = 0; i < n; i++)
        sum += arr[i];
    return sum;
}

int union_test(void)
{
    union value v;
    v.i = 0x01020304;
    return v.c[0];
}
