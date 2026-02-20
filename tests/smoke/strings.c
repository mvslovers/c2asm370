/* Smoke test: string literals and operations */

extern int strlen(const char *s);
extern int strcmp(const char *a, const char *b);
extern char *strcpy(char *dst, const char *src);

int test_strlen(void)
{
    return strlen("Hello, MVS!");
}

int test_strcmp(void)
{
    return strcmp("abc", "abd");
}

void test_strcpy(char *buf)
{
    strcpy(buf, "HLASM output");
}
