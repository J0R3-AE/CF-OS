# === Tools ===
CC	  := i686-elf-gcc
AS	  := nasm
LD	  := i686-elf-ld
OBJCOPY := i686-elf-objcopy
GRUB	:= grub-mkrescue
QEMU	:= qemu-system-i386

# === Flags ===
CFLAGS  := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra
CFLAGS  += -Iinclude -Ikernel

ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T kernel/linker.ld

# === Directories ===
SRC	 := .
BUILD   := build
ISO	 := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf

# === C Sources ===
C_SRC := $(shell find kernel -name '*.c')

# === ASM Sources ===
ASM_SRC := $(shell find kernel -name '*.asm')

USER_ELF := user/build/init.elf

# === Object Files ===
OBJ := $(patsubst %.asm,$(BUILD)/%.o,$(ASM_SRC)) \
	   $(patsubst %.c,$(BUILD)/%.o,$(C_SRC))

# === Phony Targets ===
.PHONY: all clean run iso user

# === Default ===
all:  kernel.iso

# === Compile ASM ===
$(BUILD)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# === Compile C ===
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# === Link Kernel ===
# Ensure user build runs first (produces user/build/init.elf)
$(KERNEL): user $(OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJ) -o $(KERNEL)

# === User build target ===
user:
	$(MAKE) -C user BUILD=build

$(BUILD)/kernel/user/init_elf.o: kernel/user/init_elf.asm | user
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/kernel/user/init_tar.o: kernel/user/init_tar.asm user/build/init.tar | user
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# === Build ISO ===
kernel.iso: $(KERNEL) user
	@mkdir -p $(ISO)/boot/grub
	cp $(KERNEL) $(ISO)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISO)/boot/grub/grub.cfg
	$(GRUB) -o $(BUILD)/minios.iso $(ISO)

# === Run in QEMU ===
run: kernel.iso
	$(QEMU) -cdrom $(BUILD)/minios.iso -serial stdio -hda disk.img -D qemu.log -d int

run-elf:
	$(QEMU) -kernel $(BUILD)/kernel.elf -serial stdio

# === Clean ===
clean:
	rm -rf $(BUILD)
	rm -f qemu.log
	$(MAKE) -C user clean