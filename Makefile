# === Tools ===
CC	  := i686-elf-gcc
AS	  := nasm
LD	  := i686-elf-ld
OBJCOPY := i686-elf-objcopy
GRUB	:= grub-mkrescue
QEMU	:= qemu-system-i386

# === Flags ===
CFLAGS  := -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra
CFLAGS  += -Iinclude -Ikernel -Ilib

ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T kernel/linker.ld

# === Directories ===
SRC	 := .
BUILD   := build
ISO	 := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf

# === C Sources ===
C_SRC := $(shell find kernel lib -name '*.c')

# === ASM Sources ===
ASM_SRC := $(shell find kernel -name '*.asm')


# === Object Files ===
OBJ := $(patsubst %.asm,$(BUILD)/%.o,$(ASM_SRC)) \
	   $(patsubst %.c,$(BUILD)/%.o,$(C_SRC))

# === Phony Targets ===
.PHONY: all clean run iso

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
$(KERNEL): $(OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJ) -o $(KERNEL)

# === Build ISO ===
kernel.iso: $(KERNEL)
	@mkdir -p $(ISO)/boot/grub
	cp $(KERNEL) $(ISO)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISO)/boot/grub/grub.cfg
	$(GRUB) -o $(BUILD)/minios.iso $(ISO)

disk.img:
	dd if=/dev/zero of=disk.img bs=1M count=64
	mkfs.ext2 -F disk.img

# === Run in QEMU ===
run: disk.img kernel.iso
	$(QEMU) -cdrom $(BUILD)/minios.iso -serial stdio -hda disk.img 

run-elf:
	$(QEMU) -kernel $(BUILD)/kernel.elf -serial stdio

# === Clean ===
clean: 
	rm -rf $(BUILD)
	rm -f qemu.log
