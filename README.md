# Aesc

AescOS is a small hobby operating system written in C and assembly for x86. It is currently in an early but functional stage with basic kernel services, a working interrupt system, keyboard input, terminal output, a simple VFS layer, and syscall interface.
![OS Screenshot](docs/images/Example.png)

## Current Features

### Kernel Core

- Custom bootable kernel
- Interrupt Descriptor Table (IDT) setup
- PIT timer interrupt (basic scheduling hook)
- Cooperative scheduler (thread switching via yield)

### Input System

- PS/2 keyboard driver (scancode → ASCII)
- Serial input fallback support
- Non-blocking keyboard buffer
- Basic line input via `scan()`

### Terminal / Output

- Text-mode TTY driver
- `print()` syscall for user output
- Serial mirroring for debugging output

### Filesystem (VFS)

- Virtual File System abstraction layer
- Basic vnode + file structure
- File descriptor table per process
- `open`, `close`, `read`, `write`, `readdir`

### Syscalls

Current syscall interface via `int 0x80`:

#### Process

- `SYS_exit`
- `SYS_getpid`
- `SYS_execve` (basic ELF exec stub)
- `SYS_fork` (not implemented)
- `SYS_waitpid` (not implemented)

#### File I/O

- `SYS_open`
- `SYS_close`
- `SYS_read`
- `SYS_write`
- `SYS_readdir`

#### Terminal

- `SYS_print`
- `SYS_scan` (blocking keyboard line input, still broken)

## Userland API (libc)

Minimal libc layer provides:

- `print(const char *s)`
- `scan(char *buf, int max)`
- `read(fd, buf, count)`
- `write(fd, buf, count)`
- `open(path, flags)`
- `close(fd)`
- `execve(path, argv)`
- `readdir(fd, index, out)`
- `exit(status)`

## Limitations / TODO

### Kernel

- No real process isolation yet
- No memory paging per process
- `fork()` not implemented
- `waitpid()` not implemented

### Filesystem

- VFS is basic and partially incomplete
- No persistent storage layer yet (likely ramfs / stub FS only)

### Input

- No line editing (backspace support limited)
- No arrow key handling
- No command history
- line scanning doesn't work properly

### Scheduler

- Simple cooperative scheduling only
- No preemption

### Shell

- Doesn't work at all

## Architecture Notes

- Syscalls use `int 0x80`
- Arguments passed via registers:
- EAX = syscall number
- EBX = arg1
- ECX = arg2
- EDX = arg3
- File descriptors are per-process (static table for now)
