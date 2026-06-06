# === Tools ===
CC	  := i686-elf-gcc
AS	  := nasm
LD	  := i686-elf-ld
OBJCOPY := i686-elf-objcopy
GRUB	:= grub-mkrescue
QEMU	:= qemu-system-i386

# === Flags ===
CFLAGS  := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra
CFLAGS  += -Iinclude -Isrc/kernel

ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T src/kernel/linker.ld

# === Directories ===
SRC	 := .
BUILD   := build
ISO	 := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf

# === C Sources ===
C_SRC := $(shell find src/kernel -name '*.c')

# === ASM Sources ===
ASM_SRC := $(shell find src/kernel -name '*.asm')

USER_ELF := src/user/build/init.elf

# === Object Files ===
OBJ := $(patsubst %.asm,$(BUILD)/%.o,$(ASM_SRC)) \
	   $(patsubst %.c,$(BUILD)/%.o,$(C_SRC))

# === Phony Targets ===
.PHONY: all clean run iso user install-disk-fat32 install-disk-ext2 disk-image

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
	$(MAKE) -C src/user BUILD=build

$(BUILD)/kernel/user/init_elf.o: src/kernel/user/init_elf.asm | user
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/kernel/user/init_tar.o: src/kernel/user/init_tar.asm src/user/build/init.tar | user
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# === Build ISO ===
kernel.iso: $(KERNEL) user
	@mkdir -p $(ISO)/boot/grub
	cp $(KERNEL) $(ISO)/boot/kernel.elf
	cp etc/grub.cfg $(ISO)/boot/grub/grub.cfg
	$(GRUB) -o $(BUILD)/minios.iso $(ISO)

# === Run in QEMU ===
run: kernel.iso
	$(QEMU) -hda $(BUILD)/minios.iso -serial stdio -D qemu.log -d int

run-elf:
	$(QEMU) -kernel $(BUILD)/kernel.elf -serial stdio

# === Disk Image and Installation Targets ===

disk-image:
	@mkdir -p $(BUILD)
	@python3 scripts/disk_install.py --create-disk disk.img --size-mb 64
	@echo "Disk image created: disk.img"

install-disk-fat32: kernel.iso disk-image
	@echo "Preparing bootable FAT32 disk..."
	@python3 scripts/disk_install.py --prepare-fat32 disk.img --kernel $(KERNEL)
	@echo "To complete installation, boot the ISO and run installer from within OS"

install-disk-ext2: kernel.iso disk-image
	@echo "Preparing bootable EXT2 disk..."
	@python3 scripts/disk_install.py --prepare-ext2 disk.img --kernel $(KERNEL)
	@echo "To complete installation, boot the ISO and run installer from within OS"

# === Clean ===
clean:
	rm -rf $(BUILD)
	rm -f qemu.log
	$(MAKE) -C src/user clean