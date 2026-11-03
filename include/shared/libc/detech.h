#include "types.h"
#include <stdint.h>
void arch_detect(uint32_t *arch, uint32_t *isa);
void print_cpu_features(uint32_t arch, uint32_t f);