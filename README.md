# Clearfall

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Clearfall is an experimental hobby operating system for 32-bit x86, written in
C and assembly. It includes a small kernel, a user-mode shell, basic device
drivers, and filesystem support.

## Features

- GDT, IDT, PIT, paging, cooperative scheduling, and system-call infrastructure
- PS/2 keyboard and serial input with cooked line editing
- VGA text output and an optional framebuffer terminal
- Virtual filesystem with RAMFS, FAT12/16/32, and partial EXT2 support
- FAT short-name lookup and file/directory mutation
- MBR primary-partition discovery
- User-mode shell with standalone commands packaged under `/bin`
- Up/Down command-history navigation with draft restoration
- VGA scrollback with PageUp/PageDown and a `fastfetch-esc` system summary
- File viewing with `more` pagination and `hexdump` byte inspection
- APIC, local APIC, and I/O APIC register primitives

## System calls and user library

The kernel exposes process, terminal, and file operations through `int 0x80`.
The shared user library provides wrappers for printing, line input, file I/O,
directory operations, seeking, truncation, executable launch, and waiting for
child processes.

`fork()` is not implemented. Filesystem support is limited: FAT filenames use
the 8.3 format, EXT2 support is incomplete, and there is no disk installer or
partitioning workflow. The scheduler and input paths still need broader
testing.

## Building

The build requires an `i686-elf-` cross-toolchain, NASM, GRUB utilities, and
QEMU.

```sh
make -j"$(nproc)"       # Build the bootable ISO
make kernel             # Build only the kernel ELF
make user               # Build init, command ELFs, and the init archive
make run                # Run the ISO in QEMU
make disk-test          # Run ATA PIO testing on a dedicated image
make shell-test         # Exercise shell commands in QEMU
make fat-test           # Exercise persistent FAT12 operations (requires mtools)
make debug              # Start QEMU paused with a GDB stub
make clean              # Remove generated files under build/
```

Set `CROSS_COMPILE` to select another compatible toolchain prefix, or `BUILD`
to use a different dedicated build directory.

## Known limitations

- No `fork()`, command quoting, or advanced line editing.
- Extended MBR partitions and GPT are not supported.
- FAT long filenames and EXT2 write support are not implemented.
- There is no interactive disk installer or partitioning utility.
- Scheduling, process isolation, and hardware behavior need more testing.

## Repository layout

- `src/kernel/` contains kernel code grouped by subsystem.
- `src/shared/` contains code shared by the kernel and user space.
- `src/user/` contains the shell, commands, and startup code.
- `include/kernel/`, `include/shared/`, and `include/user/` organize headers
  by ownership.
- `archive/legacy/` retains unbuilt legacy implementations.

## Contributing

Contributions, bug reports, testing, and documentation improvements are
welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Clearfall is distributed under the MIT License. See [LICENSE](LICENSE.md).
