#!/usr/bin/env python3

import argparse
import os
import re
import select
import subprocess
import sys
import time


PROMPT = re.compile(rb"\$:[^\r\n]*> \Z")


class ShellSmokeTest:
    def __init__(self, qemu, kernel, timeout, disk=None):
        self.timeout = timeout
        self.output = bytearray()
        command = [
            qemu,
            "-m",
            "256M",
            "-kernel",
            kernel,
            "-display",
            "none",
            "-serial",
            "stdio",
            "-no-reboot",
            "-no-shutdown",
        ]
        if disk:
            command.extend(["-hda", disk])
        self.process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=0,
        )

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
        try:
            self.process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait(timeout=3)

    def wait_for(self, condition, description):
        deadline = time.monotonic() + self.timeout
        while not condition():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                tail = bytes(self.output[-1000:]).decode(errors="replace")
                raise TimeoutError(
                    f"timed out waiting for {description}; serial tail:\n{tail}"
                )
            ready, _, _ = select.select(
                [self.process.stdout], [], [], min(remaining, 0.25)
            )
            if not ready:
                if self.process.poll() is not None:
                    raise RuntimeError(
                        f"QEMU exited with status {self.process.returncode}"
                    )
                continue
            data = os.read(self.process.stdout.fileno(), 4096)
            if not data:
                raise RuntimeError("QEMU serial output closed unexpectedly")
            self.output.extend(data)

    def boot(self):
        self.wait_for(lambda: PROMPT.search(self.output), "shell prompt")

    def command(self, line, expected_prompt=None):
        start = len(self.output)
        self.process.stdin.write(line.encode("ascii") + b"\r")
        self.process.stdin.flush()
        if expected_prompt is None:
            condition = lambda: PROMPT.search(self.output, start) is not None
        else:
            condition = lambda: expected_prompt in self.output[start:]
        self.wait_for(condition, f"prompt after {line!r}")
        return bytes(self.output[start:])

    def key_input(self, data, description):
        start = len(self.output)
        self.process.stdin.write(data)
        self.process.stdin.flush()
        self.wait_for(
            lambda: PROMPT.search(self.output, start) is not None,
            f"prompt after {description}",
        )
        return bytes(self.output[start:])

    def paged_command(self, line):
        start = len(self.output)
        self.process.stdin.write(line.encode("ascii") + b"\r")
        self.process.stdin.flush()
        marker = b"-- More (Enter to continue, q to quit) -- "
        self.wait_for(
            lambda: marker in self.output[start:],
            f"first page from {line!r}",
        )
        self.process.stdin.write(b"q\r")
        self.process.stdin.flush()
        self.wait_for(
            lambda: PROMPT.search(self.output, start) is not None,
            f"prompt after quitting {line!r}",
        )
        return bytes(self.output[start:])


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_test(qemu, kernel, timeout, disk=None):
    test = ShellSmokeTest(qemu, kernel, timeout, disk)
    try:
        test.boot()
        print("PASS: shell boots and displays a prompt")

        output = test.command("ver")
        require(b"AescOS version" in output, "ver output was missing")
        print("PASS: ver")

        output = test.command("fastfetch-esc")
        require(b"Clearfall / AescOS" in output and b"Directory:" in output,
                "fastfetch-esc did not show system details")
        output = test.command("fastfetch")
        require(b"Kernel:" in output, "fastfetch alias did not run")
        print("PASS: fastfetch-esc")

        for _ in range(8):
            output = test.command("ver")
            require(b"AescOS version" in output, "repeated ver failed")
        print("PASS: repeated child launch and reap")

        output = test.command("\x1b[5~\x1b[6~echo scrollback-ok")
        require(b"scrollback-ok" in output,
                "PageUp/PageDown interrupted command input")
        print("PASS: terminal PageUp/PageDown")

        output = test.key_input(b"\x1b[A\r", "Up-arrow history")
        require(b"AescOS version" in output,
                "Up arrow did not recall the previous command")
        output = test.key_input(
            b"echo draft\x1b[A\x1b[B\r", "Up/Down draft restoration"
        )
        require(b"draft\n" in output,
                "Down arrow did not restore the unfinished input")
        print("PASS: Up/Down command history")

        output = test.command("dir")
        require(b"init" in output.lower(),
                "dir did not list the root init file")
        print("PASS: dir")

        output = test.command("mkdir SMOKETST")
        require(b"failed" not in output, "mkdir reported an error")
        output = test.command("cd SMOKETST", b"$:/SMOKETST> ")
        require(b"failed" not in output, "cd reported an error")
        output = test.command("pwd")
        require(b"/SMOKETST" in output, "pwd did not report the new directory")
        print("PASS: mkdir, cd, pwd")

        output = test.command("touch EMPTY.TXT")
        require(b"failed" not in output, "touch reported an error")
        output = test.command("type EMPTY.TXT")
        require(b"failed" not in output, "type could not read an empty file")
        output = test.command("type MISSING.TXT")
        require(b"failed (error" in output,
                "external command failure status was not reported")
        print("PASS: touch, type")

        output = test.command("copy /init COPY.BIN")
        require(b"failed" not in output, "copy reported an error")
        output = test.command("dir")
        require(b"COPY.BIN" in output, "dir did not list the copied file")
        output = test.command("type COPY.BIN")
        require(b"\x7fELF" in output, "type did not print the copied ELF")
        print("PASS: copy and read-back")

        output = test.command("hexdump /init")
        require(b"7f 45 4c 46" in output,
                "hexdump did not show the ELF header")
        require(b"\n*\n" in output,
                "hexdump did not compress repeated rows")
        output = test.paged_command("more /init")
        require(b"\x7fELF" in output,
                "more did not display the first page of the file")
        print("PASS: hexdump and paged file viewing")

        output = test.command("del COPY.BIN")
        require(b"failed" not in output, "del reported an error")
        output = test.command("rm EMPTY.TXT")
        require(b"failed" not in output, "rm alias reported an error")
        output = test.command("cd ..")
        require(b"failed" not in output, "cd .. reported an error")
        output = test.command("rmdir SMOKETST")
        require(b"failed" not in output, "rmdir reported an error")
        output = test.command("mkdir SMOKERD")
        require(b"failed" not in output, "mkdir for rd test failed")
        output = test.command("rd SMOKERD")
        require(b"failed" not in output, "rd reported an error")
        output = test.command("ls")
        require(b"SMOKETST" not in output, "removed directory still appears")
        require(b"SMOKERD" not in output, "rd directory still appears")
        print("PASS: del, rm, rmdir, rd, ls")

        if disk:
            output = test.command("copy /init PERSIST.BIN")
            require(b"failed" not in output,
                    "copy to the FAT disk reported an error")
            output = test.command("copy /bin/ver PERSIST.BIN")
            require(b"failed" not in output,
                    "copy to existing FAT file reported an error")
            print("PASS: persistent FAT write and truncate")

        output = test.command("echo shell-smoke-ok")
        require(b"shell-smoke-ok" in output, "echo output was missing")
        print("PASS: echo")

        test.command("color 1F")
        output = test.command("color ZZ")
        require(b"hexadecimal digits" in output, "invalid color was accepted")
        test.command("color")
        output = test.command("cls")
        require(b"\x1b[2J\x1b[H" in output, "cls did not clear the console")
        print("PASS: color validation and cls")
    finally:
        test.close()


def main():
    parser = argparse.ArgumentParser(description="QEMU shell command smoke test")
    parser.add_argument("--qemu", default="qemu-system-i386")
    parser.add_argument("--kernel", required=True)
    parser.add_argument("--timeout", type=float, default=20)
    parser.add_argument("--disk", help="optional disk image attached as the primary drive")
    arguments = parser.parse_args()

    try:
        run_test(arguments.qemu, arguments.kernel, arguments.timeout,
                 arguments.disk)
    except (AssertionError, OSError, RuntimeError, TimeoutError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1) from error


if __name__ == "__main__":
    main()
