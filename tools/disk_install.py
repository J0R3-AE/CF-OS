#!/usr/bin/env python3
"""
Disk bootstrap and install helper for MiniOS.

This script:
1. Creates a raw disk image if needed
2. Optionally partitions and formats it with FAT32/EXT2
3. Can copy kernel and root files to disk
"""

import os
import sys
import subprocess
import struct

def create_disk_image(path, size_mb=64):
    """Create a raw disk image."""
    if os.path.exists(path):
        print(f"Disk image {path} already exists, skipping creation")
        return True
    
    print(f"Creating {size_mb}MB disk image: {path}")
    
    try:
        with open(path, 'wb') as f:
            f.write(b'\0' * (size_mb * 1024 * 1024))
        print(f"Created {path}")
        return True
    except Exception as e:
        print(f"ERROR: Failed to create disk image: {e}", file=sys.stderr)
        return False

def create_fat32_bootable(disk_path, kernel_path):
    """
    Create a minimal FAT32 filesystem on disk with boot files.
    
    This is a placeholder - actual implementation would use mtools or similar.
    For now, we document the steps needed.
    """
    print(f"\nBootable FAT32 disk preparation for {disk_path}")
    print("  (Requires mtools or parted to be installed)")
    print("\nManual steps to complete disk install:")
    print("  1. mkdosfs -F 32 -n MINIOS disk.img")
    print("  2. mcopy -i disk.img build/kernel.elf ::/kernel.elf")
    print("  3. mcopy -i disk.img user/build/init.elf ::/init")
    print("  4. Install GRUB bootloader (grub-install)")
    print("\nAlternatively, wait for OS to boot from ramfs")
    print("and run disk installer from within the OS itself.")
    return True

def create_ext2_bootable(disk_path, kernel_path):
    """
    Create a minimal EXT2 filesystem on disk with boot files.
    
    This is a placeholder - actual implementation would use mkfs.ext2.
    """
    print(f"\nBootable EXT2 disk preparation for {disk_path}")
    print("  (Requires e2fsprogs to be installed)")
    print("\nManual steps to complete disk install:")
    print("  1. mkfs.ext2 -F disk.img")
    print("  2. mount -o loop disk.img /mnt/minios")
    print("  3. cp build/kernel.elf /mnt/minios/kernel.elf")
    print("  4. cp user/build/init.elf /mnt/minios/init")
    print("  5. mkdir -p /mnt/minios/boot/grub")
    print("  6. Install GRUB bootloader")
    print("  7. umount /mnt/minios")
    print("\nAlternatively, wait for OS to boot from ramfs")
    print("and run disk installer from within the OS itself.")
    return True

def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='MiniOS disk bootstrap and install helper',
        epilog='This tool prepares raw disk images for MiniOS installation.'
    )
    
    parser.add_argument('--create-disk', metavar='PATH', 
                        help='Create a raw disk image at PATH (default: 64MB)')
    parser.add_argument('--size-mb', type=int, default=64,
                        help='Disk image size in MB (default: 64)')
    parser.add_argument('--prepare-fat32', metavar='PATH',
                        help='Prepare FAT32 filesystem on disk image')
    parser.add_argument('--prepare-ext2', metavar='PATH',
                        help='Prepare EXT2 filesystem on disk image')
    parser.add_argument('--kernel', metavar='PATH',
                        help='Kernel ELF file to copy')
    
    args = parser.parse_args()
    
    if args.create_disk:
        if not create_disk_image(args.create_disk, args.size_mb):
            sys.exit(1)
    
    if args.prepare_fat32:
        if not create_fat32_bootable(args.prepare_fat32, args.kernel):
            sys.exit(1)
    
    if args.prepare_ext2:
        if not create_ext2_bootable(args.prepare_ext2, args.kernel):
            sys.exit(1)
    
    if not (args.create_disk or args.prepare_fat32 or args.prepare_ext2):
        print("Use --help for usage information")
        sys.exit(1)

if __name__ == '__main__':
    main()
