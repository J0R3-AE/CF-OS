

extern int printf(const char *fmt, ...);
int main(void)
{
    printf("hello from userland\n");
    for (;;)
        asm volatile("hlt");
    return 0;
}