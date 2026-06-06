#include "../../libc/syscall.h"

int init_main(void);
int sh_main(void);

int main() {
    // Run init first, then shell
    init_main();
    sh_main();
    for (;;) {
    }
}