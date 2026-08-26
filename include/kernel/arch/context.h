#pragma once

#include "libk/types.h"

typedef struct
{
    u32 esp;
} context_t;

int context_create(
    context_t *ctx,
    void (*entry)(void *),
    void *arg,
    void *stack_top);

void context_switch(
    context_t *old_ctx,
    const context_t *new_ctx);