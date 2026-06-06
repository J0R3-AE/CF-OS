# MiniOS Disk Bootstrap and Installation Guide

## Overview

This guide explains the three-stage bootstrap and installation process implemented for MiniOS:

1. **Bootstrap**: Boot from ISO and run live environment
2. **Detection**: Kernel attempts to mount disk root filesystem
3. **Installation**: Init process can create/write installation marker to disk

## Current Implementation Status

### Completed (v1)
- ✅ Disk root detection in kernel (kinit.c)
- ✅ Try FAT/EXT2 mount before ramfs fallback
- ✅ Installer init app that can write to disk (ramfs or filesystem)
- ✅ Disk image creation helper script
- ✅ Makefile targets for disk preparation

### Next Steps (v2)
- ⏳ Full FAT32 filesystem creation from kernel or userland
- ⏳ Full EXT2 filesystem formatting
- ⏳ Copy kernel.elf, init, and boot files to disk
- ⏳ GRUB bootloader installation to MBR
- ⏳ Boot from installed disk (no ISO required)

## How It Works Today

### Stage 1: Bootstrap (ISO Boot)

Build and run from ISO:
```bash
make clean
make kernel.iso
make run
```

This creates `build/minios.iso` bootable via GRUB. The system:
1. Boots from CD-ROM
2. Kernel loads at `0x100000`
3. `kernel/core/kinit.c` initializes subsystems

### Stage 2: Disk Root Detection

In `kernel/core/kinit.c`, the `kernel_init()` function now:

1. **Attempts disk mount first** via `install_mount_disk_root()`
   - Tries to mount ATA primary device as root
   - Attempts FAT filesystem first
   - Falls back to EXT2 if FAT fails

2. **Falls back to ramfs** if disk unavailable
   - In-memory filesystem
   - Extracts embedded `user/build/init.tar`

### Stage 3: Installer Process

The new `user/apps/init.c` (PID 1) now:

```c
int init_main() {
    // Try to open /install_flag on disk
    int fd = open("/install_flag", 0);
    if (fd >= 0) {
        printf("Disk already installed\n");
        close(fd);
    } else {
        printf("Creating installation marker...\n");
        fd = open("/install_flag", 1);  // Create
        if (fd >= 0) {
            write(fd, "OS_INSTALLED", 12);
            close(fd);
        }
    }
    // Keep running as init
    while(1) { }
}
```

This demonstrates:
- Reading from disk (if filesystem is mounted)
- Writing to disk (to create markers/files)
- Graceful fallback if disk is read-only or ramfs-only

## Disk Image Preparation

### Create Empty Disk Image

```bash
make disk-image
```

This creates `disk.img` (64MB raw image) using the Python helper script.

### Prepare Bootable FAT32

```bash
make install-disk-fat32
```

Currently prints instructions for manual completion. To complete:

```bash
# Using mtools
mkdosfs -F 32 -n MINIOS disk.img
mcopy -i disk.img build/kernel.elf ::/kernel.elf
mcopy -i disk.img user/build/init.elf ::/init

# Install GRUB (requires grub tools)
grub-install --no-floppy --root-directory=/ -b disk.img
```

### Prepare Bootable EXT2

```bash
make install-disk-ext2
```

Currently prints instructions for manual completion. To complete:

```bash
# Using e2fsprogs
mkfs.ext2 -F disk.img

# Mount, copy files, unmount
mount -o loop disk.img /mnt/minios
mkdir -p /mnt/minios/boot/grub
cp build/kernel.elf /mnt/minios/kernel.elf
cp user/build/init.elf /mnt/minios/init
umount /mnt/minios

# Install GRUB to disk
grub-install --no-floppy --root-directory=/ -b disk.img
```

## Files Modified/Created

### Kernel Changes
- `kernel/core/kinit.c` - Disk root mounting before ramfs fallback
- `kernel/fs/install.c` - New install helper functions
- `include/fs/install.h` - Public install API
- `kernel/fs/fat/fat.c` - Made `fat_type` public (non-static)
- `kernel/fs/ext2/ext2.c` - Made `ext2_type` public (non-static)

### Userland Changes
- `user/apps/init.c` - New installer/init process with disk I/O

### Build System
- `Makefile` - New targets: `disk-image`, `install-disk-fat32`, `install-disk-ext2`
- `scripts/disk_install.py` - Python helper for disk image creation

## Testing the New System

### Test 1: Live Boot (Current)
```bash
make run
# Boots ISO, mounts ramfs, extracts tarball, runs init
# Init tries to write /install_flag
# Expected: ramfs is writable, marker created
```

### Test 2: Disk Boot (Future)
```bash
# After disk is formatted with FAT32/EXT2
qemu-system-i386 -hda disk.img -serial stdio
# Should boot from disk (needs bootloader)
# Kernel should mount FAT/EXT2 as root
# Init runs from disk
```

### Test 3: Mixed Boot (Current + Future)
```bash
make run
# Boot ISO with disk attached
# Kernel tries disk first (finds nothing)
# Falls back to ramfs
# Init still runs and marks disk as installed
# On next boot, kernel finds the marker
```

## Next Implementation Phases

### Phase 2: Full Disk Formatting
Implement `install_format_and_copy()` in `kernel/fs/install.c` to:
1. Detect disk free space
2. Create FAT32 or EXT2 filesystem
3. Write kernel and init binaries
4. Create required directories (`/boot/grub`, etc.)

### Phase 3: Bootloader Installation
Implement bootloader installation:
1. Write GRUB stage 1 to MBR (sector 0)
2. Create GRUB configuration
3. Install stage 2/1.5 loaders
4. Update partition table if needed

### Phase 4: Disk-Only Boot
Enable booting directly from disk without ISO:
1. Modify GRUB config to chainload from disk
2. Add disk boot option to Makefile
3. Test full disk installation workflow

## Architecture Decisions

### Why Try Disk First?
- Installed OS should boot faster than ISO
- Allows updates to persist across reboots
- Live environment still available if disk fails

### Why FAT Before EXT2?
- FAT32 is simpler and more universal
- Better BIOS compatibility
- Smaller filesystem overhead for minimal install

### Why Ramfs Fallback?
- Works on systems with no disk
- Always available (embedded in kernel)
- Good for development/testing
- Can extract full OS from embedded tarball

### Why Python Helper Script?
- Platform-independent disk image creation
- Can extend to full formatting/copying
- Better than hardcoding dd commands

## Security Notes

Currently unimplemented:
- No validation of disk filesystem integrity
- No authentication for install process
- No encryption of disk contents
- No rollback/recovery mechanism

These should be added in future phases.

## Troubleshooting

### Disk mount fails
- Check `build/minios.iso` boots to shell
- Verify disk.img exists and is correct size
- Look for "install_mount_disk_root" in kernel logs

### Installation marker not created
- Check if filesystem is read-only
- Verify /init has write permissions
- Check kernel logs for open/write errors

### QEMU disk not detected
- Verify `-hda disk.img` in QEMU command
- Check disk.img size is reasonable (>10MB)
- Try with `-drive file=disk.img,format=raw`

## References

- `kernel/core/kinit.c` - Main boot sequence
- `kernel/fs/install.c` - Install helpers
- `kernel/fs/blockdev.c` - Block device interface
- `kernel/hw/ata.c` - ATA/PATA driver
- `kernel/fs/fat/fat.c` - FAT filesystem
- `kernel/fs/ext2/ext2.c` - EXT2 filesystem
- `user/apps/init.c` - Init/installer process
