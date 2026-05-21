#include "../libc/syscall.h"

int init_main(void);
int sh_main(void);

int main() {
    init_main();
    return 0;
}