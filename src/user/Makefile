# User-mode build rules

CC      := i686-elf-gcc
AS      := nasm
LD      := i686-elf-ld

CFLAGS  := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra
CFLAGS += -I../../include
ASFLAGS := -f elf32

BUILD   ?= build
USER_ELF := $(BUILD)/init.elf
USER_TAR := $(BUILD)/init.tar

USER_SRCS := apps/main.c apps/init.c apps/sh.c ../libc/libc.c
USER_ASM  := crt0.asm ../libc/syscall.asm

USER_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(USER_SRCS)) \
	$(patsubst %.asm,$(BUILD)/%.o,$(USER_ASM))

.PHONY: all clean
all: $(USER_ELF) $(USER_TAR)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(USER_ELF): $(USER_OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T linker.ld $(USER_OBJS) -o $(USER_ELF)
	@echo "Built user ELF: $(USER_ELF)"

$(USER_TAR): $(USER_ELF)
	@mkdir -p $(dir $@)
	@cp $(USER_ELF) $(BUILD)/init
	@tar -C $(BUILD) -cf $(USER_TAR) init
	@rm -f $(BUILD)/init
	@echo "Built user TAR: $(USER_TAR)"

clean:
	rm -rf $(BUILD)