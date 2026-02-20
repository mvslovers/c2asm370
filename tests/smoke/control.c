/* Smoke test: control flow structures */
int test_if(int x)
{
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    else
        return 0;
}

int test_for(int n)
{
    int sum = 0;
    int i;
    for (i = 0; i < n; i++)
        sum += i;
    return sum;
}

int test_while(int n)
{
    int result = 1;
    while (n > 1) {
        result *= n;
        n--;
    }
    return result;
}

int test_switch(int x)
{
    switch (x) {
        case 0:  return 10;
        case 1:  return 20;
        case 2:  return 30;
        default: return -1;
    }
}
