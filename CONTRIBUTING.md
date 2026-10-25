# Contributing to Clearfall

Thank you for your interest in contributing to Clearfall!

Clearfall is a hobby operating system written in C and assembly for x86. It is an early-stage project focused on kernel development, device drivers, memory management, scheduling, system calls, and filesystem infrastructure.

Contributions of code, bug reports, testing, documentation, and technical discussion are welcome.

## Current Development Status

Clearfall is not yet fully functional. Some kernel subsystems have been implemented, but several still require debugging and further development.

**There is currently no working interactive shell.** The keyboard and terminal-input drivers are unreliable, and parts of the scheduler are faulty. These problems prevent the shell from reliably receiving input and resuming after input operations.

Stabilizing these foundational components is a priority before adding shell commands or expanding userland functionality.

## Areas for Contribution

### Device Drivers and Input

- Fix PS/2 keyboard input and key-event handling.
- Improve terminal input buffering and line completion.
- Fix backspace and control-character handling.
- Investigate blocking input and thread wake-up issues.

### Scheduler and Threading

- Debug cooperative scheduling and thread switching.
- Fix problems involving yielding, blocking, and waking threads.
- Improve synchronization between input drivers and waiting threads.

### Kernel and System Calls

- Improve the reliability of existing system calls.
- Continue development of process management and executable loading.
- Investigate memory management and address-space isolation.
- Improve interrupt handling and kernel subsystem integration.

### Filesystems

- Improve VFS functionality.
- Test filesystem mounting and file operations.
- Help develop RAMFS, FAT, and EXT2 integration.

### Documentation and Testing

- Improve documentation for kernel interfaces.
- Report reproducible bugs.
- Document incomplete or unreliable functionality.
- Add tests where the existing infrastructure permits them.

## Reporting Bugs

When reporting a bug, include as much of the following information as possible:

- A description of the problem.
- Steps to reproduce it.
- The expected behavior and the actual behavior.
- Relevant build output, error messages, or diagnostic logs.
- Your test environment, such as QEMU or physical hardware.

For kernel crashes, freezes, or input problems, describe exactly what happened immediately before the failure.

Please avoid reporting an assumption as a confirmed cause. Identifying the symptoms and providing reproducible steps is already useful.

## Submitting Changes

Before submitting a contribution:

1. Keep changes focused on a specific problem or improvement.
2. Follow the existing C and assembly conventions where practical.
3. Explain what changed and why.
4. Test your changes in the environments available to you.
5. Document known limitations or cases that could not be tested.

For changes involving interrupts, memory management, device drivers, or scheduling, take particular care not to introduce regressions in other subsystems.

If a feature depends on functionality that is currently broken, document that dependency instead of assuming the feature works.

## Code Guidelines

- Prefer clear, straightforward implementations over unnecessary complexity.
- Use descriptive names for functions, variables, and structures.
- Keep interfaces and responsibilities consistent with the surrounding code.
- Check buffer sizes, pointer validity, and error conditions where appropriate.
- Avoid unrelated changes when fixing a specific bug.
- Do not add diagnostic output that floods the terminal or serial console.
- Remove temporary debugging code before submitting changes, unless it is intentionally part of the solution.

Kernel code has different constraints from ordinary user-space programs. Be careful when changing interrupt handlers, shared state, memory access, and scheduling behavior.

## Testing

Test changes as thoroughly as the current state of Clearfall allows.

Depending on the change, this may include:

- Confirming that the kernel builds successfully.
- Booting the system in QEMU.
- Checking terminal and keyboard behavior.
- Verifying that existing initialization paths still work.
- Testing system calls and filesystem operations when available.

Not every subsystem is currently testable end to end. Clearly state which tests you performed and which remain unverified.

## Contribution Licensing

By submitting a contribution, you agree that your contribution may be distributed under the project's MIT License. You retain the copyright to your contribution unless otherwise agreed.

Please ensure that you have the right to submit any code or other material you contribute.

## Final Notes

Clearfall is under active development, and interfaces may change as foundational components are corrected or redesigned.

The current priority is to make the kernel's existing infrastructure reliable before expanding its features. Contributions that improve correctness, stability, and maintainability are especially valuable.

Thank you for helping improve Clearfall!
