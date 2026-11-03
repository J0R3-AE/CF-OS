<<<<<<< HEAD
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
=======
# Cross-compilation tools. Override CROSS_COMPILE to use another toolchain.
CROSS_COMPILE ?= i686-elf-
CC            := $(CROSS_COMPILE)gcc
CXX           := $(CROSS_COMPILE)g++
LD            := $(CROSS_COMPILE)ld
AS            := nasm
GRUB_MKRESCUE ?= grub-mkrescue
QEMU          ?= qemu-system-i386
QEMU_IMG      ?= qemu-img

BUILD ?= build

ifneq ($(filter / . ./ ./.. ./. .. ../% /% %/.. %/../%,$(BUILD)),)
$(error BUILD must be a dedicated relative directory beneath the repository)
endif

CPPFLAGS += -Iinclude -Iinclude/shared -Isrc/kernel -Isrc/shared -Isrc/user
CFLAGS   ?=
CFLAGS   += -m32 -ffreestanding -fno-builtin -O2 -Wall -Wextra
CXXFLAGS += $(CFLAGS)
ASFLAGS  ?= -f elf32

KERNEL_LDFLAGS ?= -m elf_i386 -T src/kernel/linker.ld
USER_LDFLAGS   ?= -m elf_i386 -T src/user/linker.ld
QEMUFLAGS      ?= -m 512M -serial stdio
ATA_PIO_SELFTEST ?= 0

ifeq ($(ATA_PIO_SELFTEST),1)
CPPFLAGS += -DATA_PIO_SELFTEST
endif

>>>>>>> 0314c3a (	modified:   Makefile)
KERNEL_BUILD := $(BUILD)/kernel
SHARED_BUILD := $(BUILD)/shared
USER_BUILD   := $(BUILD)/user
ISO_ROOT     := $(BUILD)/iso
KERNEL       := $(BUILD)/kernel.elf
USER_ELF     := $(BUILD)/init.elf
USER_TAR     := $(BUILD)/init.tar
ISO_IMAGE    := $(BUILD)/kernel.iso
USER_COMMANDS := dir type copy touch mkdir del rd ver
USER_BIN_ELFS := $(addprefix $(USER_BUILD)/bin/,$(USER_COMMANDS))

KERNEL_C_SRC := $(shell find src/kernel -type f -name '*.c' -print | sort)
KERNEL_ASM_SRC := $(shell find src/kernel -type f -name '*.asm' -print | sort)
SHARED_C_SRC   := $(shell find src/shared -type f -name '*.c' -print | sort)
SHARED_ASM_SRC := $(shell find src/shared -type f -name '*.asm' -print | sort)
USER_C_SRC   := $(shell find src/user -type f -name '*.c' -print | sort)
USER_C_SRC   := $(filter-out src/user/bin/%.c,$(USER_C_SRC))
USER_CPP_SRC := $(shell find src/user -type f -name '*.cpp' -print | sort)
USER_ASM_SRC := $(shell find src/user -type f -name '*.asm' -print | sort)

<<<<<<< HEAD
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
	$(patsubst src/kernel/%.asm,$(KERNEL_BUILD)/%_asm.o,$(KERNEL_ASM_SRC)) \
	$(patsubst src/kernel/%.c,$(KERNEL_BUILD)/%.o,$(KERNEL_C_SRC))

# Strip src/user/ prefix
USER_OBJS := \
=======
KERNEL_OBJ := \
	$(patsubst src/kernel/%.c,$(KERNEL_BUILD)/%.o,$(KERNEL_C_SRC)) \
	$(patsubst src/kernel/%.asm,$(KERNEL_BUILD)/%.asm.o,$(KERNEL_ASM_SRC))
SHARED_OBJ := \
	$(patsubst src/shared/%.c,$(SHARED_BUILD)/%.o,$(SHARED_C_SRC)) \
	$(patsubst src/shared/%.asm,$(SHARED_BUILD)/%.o,$(SHARED_ASM_SRC))
USER_OBJ := \
>>>>>>> 0314c3a (	modified:   Makefile)
	$(patsubst src/user/%.c,$(USER_BUILD)/%.o,$(USER_C_SRC)) \
	$(patsubst src/user/%.cpp,$(USER_BUILD)/%.o,$(USER_CPP_SRC)) \
	$(patsubst src/user/%.asm,$(USER_BUILD)/%.o,$(USER_ASM_SRC))

# Keep the user runtime explicit; linking every shared object would pull in
# kernel-only library code that is not part of the user ABI.
USER_SHARED_ASM_SRC := \
	src/shared/libc/asm/math.asm \
	src/shared/libc/asm/mem.asm \
	src/shared/libc/asm/string.asm \
	src/shared/libc/asm/syscall.asm
USER_SHARED_C_SRC := src/shared/libc/syscall.c
USER_SHARED_OBJ := \
	$(patsubst src/shared/%.asm,$(SHARED_BUILD)/%.o,$(USER_SHARED_ASM_SRC)) \
	$(patsubst src/shared/%.c,$(SHARED_BUILD)/%.o,$(USER_SHARED_C_SRC))

DEPFILES := \
	$(patsubst src/kernel/%.c,$(KERNEL_BUILD)/%.d,$(KERNEL_C_SRC)) \
	$(patsubst src/shared/%.c,$(SHARED_BUILD)/%.d,$(SHARED_C_SRC)) \
	$(patsubst src/user/%.c,$(USER_BUILD)/%.d,$(USER_C_SRC)) \
	$(patsubst src/user/%.cpp,$(USER_BUILD)/%.d,$(USER_CPP_SRC))

<<<<<<< HEAD
# === shared ===
=======
.PHONY: all kernel user iso run run-disk disk-test shell-test fat-test debug clean help

all: iso

kernel: $(KERNEL)

user: $(USER_ELF) $(USER_BIN_ELFS) $(USER_TAR)

iso: $(ISO_IMAGE)

$(BUILD):
	mkdir -p $@

>>>>>>> 0314c3a (	modified:   Makefile)
$(SHARED_BUILD)/%.o: src/shared/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

<<<<<<< HEAD
$(SHARED_BUILD)%.o: src/shared/%.asm
=======
$(SHARED_BUILD)/%.o: src/shared/%.asm
>>>>>>> 0314c3a (	modified:   Makefile)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL_BUILD)/%.o: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

$(KERNEL_BUILD)/%_asm.o: src/kernel/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

<<<<<<< HEAD

$(KERNEL): $(SHARED_OBJ) $(USER_ELF) $(USER_TAR) $(KERNEL_OBJ)
	@mkdir -p $(dir $@)
	# Use sort to remove duplicate object entries that can cause multiple-definition
	# linker errors when the same object path appears more than once.
	$(LD) $(LDFLAGS) $(sort $(KERNEL_OBJ) $(SHARED_OBJ)) -o $@
	@echo "Built kernel ELF: $@"

# === User ===
=======
>>>>>>> 0314c3a (	modified:   Makefile)
$(USER_BUILD)/%.o: src/user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

$(USER_BUILD)/%.o: src/user/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

$(USER_BUILD)/%.o: src/user/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(USER_BUILD)/bin/entry-%.o: src/user/bin/entry.c
	@mkdir -p $(dir $@)
<<<<<<< HEAD
	$(LD) -m elf_i386 -T src/user/linker.ld $(USER_OBJS) $(USHARED_OBJ) -o $@
	@echo "Built user ELF: $@"
=======
	$(CC) $(CPPFLAGS) $(CFLAGS) -DAPPLET_ENTRY=cmd_$* -c $< -o $@
>>>>>>> 0314c3a (	modified:   Makefile)

$(USER_BUILD)/bin/%: $(USER_BUILD)/bin/%.o $(USER_BUILD)/bin/entry-%.o \
		$(USER_BUILD)/crt0.o \
		$(USER_SHARED_OBJ) src/user/linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) $(USER_BUILD)/crt0.o \
		$(USER_BUILD)/bin/entry-$*.o \
		$(USER_BUILD)/bin/$*.o $(USER_SHARED_OBJ) -o $@

<<<<<<< HEAD
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

=======
# The assembly images use incbin, so these dependencies are required for
# correct parallel builds as well as rebuilds when an embedded image changes.
$(KERNEL_BUILD)/user/init_elf.asm.o: src/kernel/user/init_elf.asm $(USER_ELF)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -DUSER_INIT_ELF='"$(USER_ELF)"' $< -o $@

$(KERNEL_BUILD)/user/init_tar.asm.o: src/kernel/user/init_tar.asm $(USER_TAR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -DUSER_INIT_TAR='"$(USER_TAR)"' $< -o $@

$(KERNEL): $(KERNEL_OBJ) $(SHARED_OBJ) src/kernel/linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJ) $(SHARED_OBJ) -o $@

$(USER_ELF): $(USER_OBJ) $(USER_SHARED_OBJ) src/user/linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) $(USER_OBJ) $(USER_SHARED_OBJ) -o $@

$(USER_TAR): $(USER_ELF) $(USER_BIN_ELFS)
	@mkdir -p $(BUILD)/rootfs/bin
	cp $< $(BUILD)/rootfs/init
	for command in $(USER_COMMANDS); do \
		cp $(USER_BUILD)/bin/$$command $(BUILD)/rootfs/bin/$$command; \
	done
	tar -C $(BUILD)/rootfs -cf $@ init bin

$(ISO_ROOT)/boot/kernel.elf: $(KERNEL)
	@mkdir -p $(dir $@)
	cp $< $@

$(ISO_ROOT)/boot/init.tar: $(USER_TAR)
	@mkdir -p $(dir $@)
	cp $< $@

$(ISO_ROOT)/boot/grub/grub.cfg: boot/grub/grub.cfg
	@mkdir -p $(dir $@)
	cp $< $@

$(ISO_IMAGE): $(ISO_ROOT)/boot/kernel.elf $(ISO_ROOT)/boot/init.tar \
		$(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

run: $(ISO_IMAGE)
	$(QEMU) $(QEMUFLAGS) -cdrom $(ISO_IMAGE)

run-disk: $(ISO_IMAGE)
	$(QEMU) $(QEMUFLAGS) -boot d -cdrom $(ISO_IMAGE) -hda $(BUILD)/disk.img

disk-test:
	@mkdir -p build/ata-selftest
	@if [ ! -e build/ata-selftest/disk.img ]; then \
		$(QEMU_IMG) create -f raw build/ata-selftest/disk.img 64M; \
	fi
	$(MAKE) BUILD=build/ata-selftest ATA_PIO_SELFTEST=1 iso
	$(QEMU) $(QEMUFLAGS) -boot d -cdrom build/ata-selftest/kernel.iso \
		-hda build/ata-selftest/disk.img

shell-test: $(KERNEL)
	python3 tests/shell_smoke.py --qemu "$(QEMU)" --kernel "$(KERNEL)"

fat-test: $(KERNEL) $(USER_ELF) $(USER_BIN_ELFS)
	python3 tests/fat_smoke.py --qemu "$(QEMU)" --kernel "$(KERNEL)" \
		--init "$(USER_ELF)" --bin-dir "$(USER_BUILD)/bin"

debug: $(ISO_IMAGE)
	$(QEMU) $(QEMUFLAGS) -cdrom $(ISO_IMAGE) -S -s
>>>>>>> 0314c3a (	modified:   Makefile)

clean:
	rm -rf -- "$(BUILD)"

help:
	@echo "Build targets:"
	@echo "  all (default)  Build the bootable ISO in $(BUILD)/kernel.iso"
	@echo "  kernel         Build the kernel ELF"
	@echo "  user           Build the user ELF, command ELFs, and init archive"
	@echo "  iso            Build the bootable ISO"
	@echo "  run            Boot the ISO in QEMU"
	@echo "  run-disk       Run the ISO with $(BUILD)/disk.img as the primary disk"
	@echo "  disk-test      Run ATA read/write test on a dedicated test image"
	@echo "  shell-test     Boot QEMU and smoke-test shell commands"
	@echo "  fat-test       Smoke-test persistent FAT12 file operations (requires mtools)"
	@echo "  debug          Boot QEMU paused with a GDB stub on port 1234"
	@echo "  clean          Remove generated files under $(BUILD)/"
	@echo "Variables:"
	@echo "  CROSS_COMPILE  Cross-toolchain prefix (default: i686-elf-)"
	@echo "  BUILD          Build output directory (default: build)"

-include $(DEPFILES)
