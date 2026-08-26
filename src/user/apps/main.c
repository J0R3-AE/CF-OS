// user/app/main.c

#include <stdint.h>
#include "syscall.h"
#include "libc/string.h"

int shell_main(void);

static const char *art =
    "###########################################################\n"
    "#                                                         #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  ###################                        ##########  #\n"
    "#  ######################        ########     ##########  #\n"
    "#  ######################  #     ###########  ##########  #\n"
    "#  ####################   ##     ###########  ##########  #\n"
    "#  ####################  ###     #######################  #\n"
    "#  ###################  ####     #######################  #\n"
    "#  ##################  #####     ########  #############  #\n"
    "#  #################  ######               #############  #\n"
    "#  ################  #######     #######   #############  #\n"
    "#  ##############                ########  #############  #\n"
    "#  #############    ########     #######################  #\n"
    "#  #############  ##########     #######################  #\n"
    "#  ###########    ##########     ##############  #######  #\n"
    "#  ###########   ###########     ############   ########  #\n"
    "#  #########    ############     ##########     ########  #\n"
    "#  #######        ########                    ##########  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#  #####################################################  #\n"
    "#                                                         #\n"
    "###########################################################\n";

static inline void cpuid(uint32_t leaf,
                         uint32_t *eax,
                         uint32_t *ebx,
                         uint32_t *ecx,
                         uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf));
}

static inline void cpuid_ex(uint32_t leaf, uint32_t subleaf,
                            uint32_t *eax,
                            uint32_t *ebx,
                            uint32_t *ecx,
                            uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf));
}

static const char frames[4] = {'|', '/', '-', '\\'};

void arch_detect(uint32_t *arch, uint32_t *features)
{
    uint32_t a, b, c, d;

    // You are running x86 code → architecture is x86
    *arch = 1;

    // Check CPUID support
    uint32_t eflags;
    __asm__ volatile("pushf; pop %0" : "=r"(eflags));

    uint32_t toggled = eflags ^ (1 << 21);
    __asm__ volatile("push %0; popf" ::"r"(toggled));

    uint32_t newflags;
    __asm__ volatile("pushf; pop %0" : "=r"(newflags));

    if (((newflags ^ eflags) & (1 << 21)) == 0)
    {
        *features = 0;
        return;
    }

    // Extended CPUID leaf availability
    cpuid(0x80000000, &a, &b, &c, &d);
    uint32_t max_ext = a;

    // Detect x86_64 long mode
    if (max_ext >= 0x80000001)
    {
        cpuid(0x80000001, &a, &b, &c, &d);
        if (d & (1 << 29))
        {
            *arch = 2; // x86_64 capable
        }
    }

    // Basic feature bits
    cpuid(1, &a, &b, &c, &d);

    uint32_t f = 0;

    // Classic features
    if (d & (1 << 23))
        f |= (1 << 0); // MMX
    if (d & (1 << 25))
        f |= (1 << 1); // SSE
    if (d & (1 << 26))
        f |= (1 << 2); // SSE2

    // Extended features
    if (c & (1 << 0))
        f |= (1 << 3); // SSE3
    if (c & (1 << 9))
        f |= (1 << 4); // SSSE3
    if (c & (1 << 19))
        f |= (1 << 5); // SSE4.1
    if (c & (1 << 20))
        f |= (1 << 6); // SSE4.2
    if (c & (1 << 25))
        f |= (1 << 7); // AES
    if (c & (1 << 28))
        f |= (1 << 8); // AVX
    if (c & (1 << 23))
        f |= (1 << 9); // POPCNT
    if (c & (1 << 30))
        f |= (1 << 10); // RDRAND

    // Structured extended features (leaf 7)
    cpuid_ex(7, 0, &a, &b, &c, &d);

    if (b & (1 << 5))
        f |= (1 << 11); // AVX2
    if (b & (1 << 3))
        f |= (1 << 12); // BMI1
    if (b & (1 << 8))
        f |= (1 << 13); // BMI2
    if (b & (1 << 12))
        f |= (1 << 14); // FMA
    if (b & (1 << 18))
        f |= (1 << 15); // RDSEED
    if (b & (1 << 29))
        f |= (1 << 16); // SHA

    *features = f;
}

void print_cpu_features(uint32_t arch, uint32_t f)
{
    print("CPU Architecture: ");
    switch (arch)
    {
    case 1:
        print("x86 (32-bit)\n");
        break;
    case 2:
        print("x86_64 capable (64-bit)\n");
        break;
    default:
        print("Unknown\n");
        break;
    }

    print("Supported ISA Features:\n");

    for (int i = 0; i <= 16; i++)
    {
        if (!(f & (1 << i)))
            continue;

        switch (i)
        {
        case 0:
            print(" - MMX\n");
            break;
        case 1:
            print(" - SSE\n");
            break;
        case 2:
            print(" - SSE2\n");
            break;
        case 3:
            print(" - SSE3\n");
            break;
        case 4:
            print(" - SSSE3\n");
            break;
        case 5:
            print(" - SSE4.1\n");
            break;
        case 6:
            print(" - SSE4.2\n");
            break;
        case 7:
            print(" - AES\n");
            break;
        case 8:
            print(" - AVX\n");
            break;
        case 9:
            print(" - POPCNT\n");
            break;
        case 10:
            print(" - RDRAND\n");
            break;
        case 11:
            print(" - AVX2\n");
            break;
        case 12:
            print(" - BMI1\n");
            break;
        case 13:
            print(" - BMI2\n");
            break;
        case 14:
            print(" - FMA\n");
            break;
        case 15:
            print(" - RDSEED\n");
            break;
        case 16:
            print(" - SHA Extensions\n");
            break;
        default:
            break;
        }
    }
}

void main(void)
{
    print(art);
    uint32_t arch = 0;
    uint32_t isa = 0;
    arch_detect(&arch, &isa);
    print_cpu_features(arch, isa);
    print("Hello, World, From Usermode!\n");
    print("This is a simple demo of a usermode application.\n");
    print("It will print a spinning dial to show that it's alive.\n");
    print("Press Ctrl+C to exit.\n");
    int i = 0;
    
    shell_main();
    while (1)
    {
        char buf[2] = {frames[i % 4], '\0'};

        print("\r");
        print(buf);

        i++;
    }
}