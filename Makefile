# === Tools ===
CC	  := i686-elf-gcc
CXX	  := i686-elf-g++
AS	  := nasm
LD	  := i686-elf-ld
OBJCOPY := i686-elf-objcopy
GRUB	:= grub-mkrescue
QEMU	:= qemu-system-i386

# === Flags ===
CFLAGS  := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra -Iinclude -Isrc/kernel -Isrc/libc -Isrc/user

ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T src/kernel/linker.ld

# === Directories ===
SRC	 := .
GRUB_DIR := boot/grub
BUILD   := build
LIBC_BUILD := $(BUILD)/libc
KERNEL_BUILD := $(BUILD)/kernel
USER_BUILD := $(BUILD)/user

ISO	 := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf

# === Libc C & ASM Sources ===
LIBC_C_SRC := $(shell find src/libc -name '*.c')
LIBC_ASM_SRC := $(shell find src/libc -name '*.asm')

# === Kernel C & ASM Sources ===
KERNEL_C_SRC := $(shell find src/kernel -name '*.c')
KERNEL_ASM_SRC := $(shell find src/kernel -name '*.asm')

# === User C, C++, and ASM Sources ===
USER_C_SRC := $(shell find src/user -name '*.c')
USER_CPP_SRC := $(shell find src/user -name '*.cpp')
USER_ASM_SRC := $(shell find src/user -name '*.asm')

USER_ELF := $(BUILD)/init.elf
USER_TAR := $(BUILD)/init.tar

# === Object Files ===
LIBC_OBJ := \
	$(patsubst src/libc/%.asm,$(LIBC_BUILD)/%.o,$(LIBC_ASM_SRC)) \
	$(patsubst src/libc/%.c,$(LIBC_BUILD)/%.o,$(LIBC_C_SRC))

ULIBC_OBJ := \
    build/libc/asm/math.o \
    build/libc/asm/mem.o \
    build/libc/asm/syscall.o \
	build/libc/syscall.o \

# Strip src/kernel/ prefix
KERNEL_OBJ := \
	$(patsubst src/kernel/%.asm,$(KERNEL_BUILD)/%.o,$(KERNEL_ASM_SRC)) \
	$(patsubst src/kernel/%.c,$(KERNEL_BUILD)/%.o,$(KERNEL_C_SRC))

# Strip src/user/ prefix
USER_OBJS := \
	$(patsubst src/user/%.c,$(USER_BUILD)/%.o,$(USER_C_SRC)) \
	$(patsubst src/user/%.cpp,$(USER_BUILD)/%.o,$(USER_CPP_SRC)) \
	$(patsubst src/user/%.asm,$(USER_BUILD)/%.o,$(USER_ASM_SRC))

# === Default ===
all: kernel.iso


# === Libc ===
$(LIBC_BUILD)/%.o: src/libc/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBC_BUILD)%.o: src/libc/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
	
# === Kernel ===
$(KERNEL_BUILD)/%.o: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD)/%.o: src/kernel/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(LIBC_OBJ) $(USER_ELF) $(USER_TAR) $(KERNEL_OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(KERNEL_OBJ) $(LIBC_OBJ) -o $@
	@echo "Built kernel ELF: $@"

# === User ===
$(USER_BUILD)/%.o: src/user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(USER_BUILD)/%.o: src/user/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(USER_BUILD)/%.o: src/user/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(USER_ELF): $(USER_OBJS) src/user/linker.ld
	@mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T src/user/linker.ld $(USER_OBJS) $(ULIBC_OBJ) -o $@
	@echo "Built user ELF: $@"

$(USER_TAR): $(USER_ELF)
	@cp $(USER_ELF) $(BUILD)/init
	@tar -C $(BUILD) -cf $@ init
	@rm -f $(BUILD)/init
	@echo "Built user TAR: $@"

# === ISO Image ===
kernel.iso: $(KERNEL) $(USER_TAR)
	@mkdir -p $(ISO)/boot/grub
	@cp $(KERNEL) $(ISO)/boot/kernel.elf
	@cp $(USER_TAR) $(ISO)/boot/init.tar
	@cp $(GRUB_DIR)/grub.cfg $(ISO)/boot/grub/grub.cfg
	$(GRUB) -o $@ $(ISO)
	@echo "Built ISO image: $@"

run: kernel.iso
	$(QEMU) -cdrom $< -m 512M -serial stdio

clean:
	rm -rf $(BUILD)
	rm -f qemu.log disk.img