# User-mode build rules

CC      := i686-elf-gcc
AS      := nasm

CFLAGS  := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra
CFLAGS  += -I../../include

CXXFLAGS := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra \
            -fno-exceptions -fno-rtti -fno-use-cxa-atexit \
            -I../../include

ASFLAGS := -f elf32

BUILD   ?= build
USER_ELF := $(BUILD)/init.elf
USER_TAR := $(BUILD)/init.tar

# Add .cpp files here
USER_SRCS := \
    apps/main.c \
    apps/init.c \
    apps/sh.cpp \
    ../libc/libc.c

USER_CPP  := \
    apps/sh.cpp

USER_ASM  := \
    crt0.asm \
    ../libc/syscall.asm

USER_OBJS := \
    $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(USER_SRCS))) \
    $(patsubst %.cpp,$(BUILD)/%.o,$(filter %.cpp,$(USER_SRCS))) \
    $(patsubst %.asm,$(BUILD)/%.o,$(USER_ASM))

.PHONY: all clean
all: $(USER_ELF) $(USER_TAR)

# C files
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# C++ files
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ASM files
$(BUILD)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link step (use g++)
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