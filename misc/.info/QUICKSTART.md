# Quick Start: Testing the New Install System

## Build the Project
```bash
cd /mnt/m/Code/minios
make clean
make
```

This creates:
- `build/minios.iso` - Bootable ISO image
- `user/build/init.elf` - Init/installer binary
- `user/build/init.tar` - Embedded root filesystem

## Test 1: Boot from ISO (Live Environment)
```bash
make run
```

Expected output in QEMU console:
```
...
Kernel initializing...
...
install_mount_disk_root: trying to mount disk root
install_mount_disk_root: no ATA device found
install_mount_disk_root: no valid filesystem found on disk
Disk root not available, falling back to ramfs
RAMFS initialized
Extracting embedded init.tar into ramfs (size=...)
Attempting vfs lookup for /init
Usermode init process scheduled from /init (ramfs)
...
===== MiniOS Installer/Init Process =====
Init PID: 1
Checking for existing disk installation...
...
```

The init process will try to write `/install_flag` to the ramfs (which is writable on first boot).

## Test 2: Create Disk Image
```bash
make disk-image
```

Creates `disk.img` (64MB raw image).

Verify it was created:
```bash
ls -lh disk.img
file disk.img
```

## Test 3: Boot with Disk Attached
```bash
qemu-system-i386 \
  -cdrom build/minios.iso \
  -hda disk.img \
  -serial stdio \
  -m 256
```

Or use the modified `make run` target (which already includes `-hda disk.img`).

Expected behavior:
1. Boot from ISO
2. Kernel tries to detect disk filesystem (will fail on empty disk)
3. Falls back to ramfs
4. Init runs and can still write to disk (for markers/metadata)

## Test 4: Verify Disk Mount Logic
Look in kernel logs for:
```
install_mount_disk_root: trying to mount disk root
```

If disk has FAT/EXT2:
```
install_mount_disk_root: FAT root mounted successfully
```

If disk is empty (current state):
```
install_mount_disk_root: no valid filesystem found on disk
Disk root not available, falling back to ramfs
```

## Next Steps: Complete the Installation

After v1 testing, implement v2:

### Manual Disk Preparation (Testing v2)
```bash
# Create FAT32 filesystem on disk
mkdosfs -F 32 -n MINIOS disk.img

# Mount and add files
mkdir -p /tmp/minios_mount
sudo mount -o loop disk.img /tmp/minios_mount
mkdir -p /tmp/minios_mount/boot
cp build/kernel.elf /tmp/minios_mount/kernel.elf
cp user/build/init.elf /tmp/minios_mount/init
sudo umount /tmp/minios_mount
```

Then boot with the prepared disk:
```bash
make run  # Uses disk.img with actual filesystem
```

Expected v2 behavior:
1. Boot from ISO
2. Kernel detects FAT filesystem on disk
3. Mounts disk as root
4. Init runs from disk (not ramfs)
5. All files available from disk

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│  QEMU (or Real Hardware)                                 │
├──────────────────┬──────────────────┬──────────────────┐
│  build/          │  disk.img        │  Bootloader      │
│  minios.iso      │  (empty in v1)   │  (GRUB via CD)   │
│  (GRUB CD)       │                  │                  │
└──────────────────┴──────────────────┴──────────────────┘
         │                  │
         └──────────────────┘
                  │
         ┌────────▼─────────┐
         │ Kernel Boot      │
         ├──────────────────┤
         │ kinit.c:         │
         │ 1. Try disk      │
         │ 2. Fall back to  │
         │    ramfs         │
         └────────┬─────────┘
                  │
         ┌────────▼──────────────────────┐
         │ filesystem mount (v1: ramfs)  │
         │ or disk FAT/EXT2 (v2+)       │
         └────────┬──────────────────────┘
                  │
         ┌────────▼──────────────────────┐
         │ Init/Installer Process        │
         │ (user/apps/init.c)            │
         │ - Detect installation         │
         │ - Write markers               │
         │ - (future: format & populate) │
         └───────────────────────────────┘
```

## Troubleshooting

### ISO won't boot
```bash
make clean
make kernel.iso
file build/minios.iso  # Should say "ISO 9660 ... bootable"
```

### Init doesn't appear
Check kernel logs for:
- `vfs_lookup(/init)` - should succeed
- `exec_elf_vnode` - should return 0

### Disk operations fail
- Check kernel logs for `install_mount_disk_root`
- Verify `blockdev_open("ata0")` succeeds
- Check ATA driver output

### QEMU shows no output
```bash
make run  # Should show `-serial stdio` in QEMU command
```

## Clean Rebuild
```bash
make clean && make && make run
```

All commands above work with this workflow.

## Files to Examine

After running, check:
- `qemu_stdout.txt` - Kernel output log
- `build/minios.iso` - Bootable ISO
- `build/kernel.elf` - Kernel binary
- `user/build/init.elf` - Init binary
- `disk.img` - Disk image (if created)

## Key Implementation Files

- `kernel/core/kinit.c` - Boot sequence
- `kernel/fs/install.c` - Disk detection
- `user/apps/init.c` - Init/installer process
- `.info/INSTALL.md` - Full documentation
- `.info/IMPLEMENTATION.md` - Technical details
