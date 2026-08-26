#pragma once

#include "libc/types.h"

typedef struct
{
    u32 esp;
} context_t;


int context_create(
    context_t *context,
    void (*entry)(void *),
    void *arg,
    void *stack_top);

void context_switch(
    context_t *old_context,
    const context_t *new_context);