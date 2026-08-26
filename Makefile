export PATH := $(HOME)/.local/i386-elf/bin:$(PATH)
# === Tools ===
CC	  := i386-elf-gcc
CXX	  := i386-elf-g++
AS	  := nasm
LD	  := i386-elf-ld
OBJCOPY := i386-elf-objcopy
GRUB	:= grub-mkrescue
QEMU	:= qemu-system-i386

# === Flags ===
CFLAGS  := -ffreestanding -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -Iinclude -Isrc/kernel -Isrc/shared -Isrc/user
CXXFLAGS := $(CFLAGS)

ASFLAGS := -f elf32
LDFLAGS := -T src/kernel/linker.ld

# === Directories ===
SRC	 := .
GRUB_DIR := boot/grub
BUILD   := build
SHARED_BUILD := $(BUILD)/shared
KERNEL_BUILD := $(BUILD)/kernel
USER_BUILD := $(BUILD)/user

ISO	 := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf

# === shared C & ASM Sources ===
SHARED_C_SRC := $(shell find src/shared -name '*.c')
SHARED_ASM_SRC := $(shell find src/shared -name '*.asm')

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
SHARED_OBJ := \
	$(patsubst src/shared/%.asm,$(SHARED_BUILD)/%.o,$(SHARED_ASM_SRC)) \
	$(patsubst src/shared/%.c,$(SHARED_BUILD)/%.o,$(SHARED_C_SRC))

USHARED_OBJ := \
    build/shared/asm/math.o \
    build/shared/asm/mem.o \
    build/shared/asm/syscall.o \
	build/shared/asm/string.o \
	build/shared/syscall.o \

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


# === shared ===
$(SHARED_BUILD)/%.o: src/shared/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(SHARED_BUILD)%.o: src/shared/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
	
# === Kernel ===
$(KERNEL_BUILD)/%.o: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BUILD)/%.o: src/kernel/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(SHARED_OBJ) $(USER_ELF) $(USER_TAR) $(KERNEL_OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(KERNEL_OBJ) $(SHARED_OBJ) -o $@
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
	$(LD) -m elf_i386 -T src/user/linker.ld $(USER_OBJS) $(USHARED_OBJ) -o $@
	@echo "Built user ELF: $@"

$(USER_TAR): $(USER_ELF)
	@cp $(USER_ELF) $(BUILD)/init
	@tar -C $(BUILD) -cf $@ init
	@rm -f $(BUILD)/init
	@echo "Built user TAR: $@"

build/kernel/user/init_tar.o: $(USER_TAR)

build/kernel/user/init_elf.o: $(USER_ELF)

# === ISO Image ===
kernel.iso: $(KERNEL) $(USER_TAR)
	@mkdir -p $(ISO)/boot/grub
	@cp $(KERNEL) $(ISO)/boot/kernel.elf
	@cp $(USER_TAR) $(ISO)/boot/init.tar
	@cp $(GRUB_DIR)/grub.cfg $(ISO)/boot/grub/grub.cfg
	$(GRUB) -o $@ $(ISO)
	@echo "Built ISO image: $@"

run: kernel.iso
	qemu-system-i386 -cdrom kernel.iso -m 512M -serial stdio -display gtk

debug-path:
	@echo "Current PATH inside make is:"
	@echo $(PATH)
	@which i686-elf-tools || echo "i686-elf-gcc NOT FOUND IN MAKE PATH"

full:
	@echo "Cleaning previous build..."
	@$(MAKE) clean
	@echo "Building full project..."
	@$(MAKE) all
	@echo "Running QEMU..."
	@$(MAKE) run


clean:
	rm -rf $(BUILD)
	rm -f qemu.log disk.img