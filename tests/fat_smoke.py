#!/usr/bin/env python3

import argparse
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from shell_smoke import ShellSmokeTest, require, run_test


SECTOR_SIZE = 512
SECTOR_COUNT = 2880


def create_fat12_image(image, init_elf, bin_dir):
    volume = bytearray(SECTOR_SIZE * SECTOR_COUNT)
    boot = memoryview(volume)[:SECTOR_SIZE]
    boot[0:3] = b"\xeb\x3c\x90"
    boot[3:11] = b"AESCOS  "
    struct.pack_into("<H", boot, 11, SECTOR_SIZE)
    boot[13] = 1
    struct.pack_into("<H", boot, 14, 1)
    boot[16] = 2
    struct.pack_into("<H", boot, 17, 224)
    struct.pack_into("<H", boot, 19, SECTOR_COUNT)
    boot[21] = 0xF0
    struct.pack_into("<H", boot, 22, 9)
    struct.pack_into("<H", boot, 24, 18)
    struct.pack_into("<H", boot, 26, 2)
    boot[43:54] = b"AESCOS TEST"
    boot[54:62] = b"FAT12   "
    boot[510:512] = b"\x55\xaa"
    fat = bytes((0xF0, 0xFF, 0xFF)) + bytes(9 * SECTOR_SIZE - 3)
    volume[SECTOR_SIZE:SECTOR_SIZE + len(fat)] = fat
    volume[10 * SECTOR_SIZE:19 * SECTOR_SIZE] = fat
    image.write_bytes(volume)

    commands = [
        ["mmd", "-i", str(image), "::/bin"],
        ["mcopy", "-i", str(image), str(init_elf), "::/init"],
    ]
    for executable in sorted(bin_dir.iterdir()):
        if executable.is_file():
            commands.append(
                ["mcopy", "-i", str(image), str(executable),
                 f"::/bin/{executable.name}"]
            )
    for command in commands:
        subprocess.run(command, check=True, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE)


def main():
    parser = argparse.ArgumentParser(
        description="QEMU FAT12 persistent file-operation smoke test"
    )
    parser.add_argument("--qemu", default="qemu-system-i386")
    parser.add_argument("--kernel", required=True)
    parser.add_argument("--init", required=True)
    parser.add_argument("--bin-dir", required=True)
    parser.add_argument("--timeout", type=float, default=20)
    arguments = parser.parse_args()

    required_tools = ("mcopy", "mmd", "mtype")
    missing = [tool for tool in required_tools if not shutil.which(tool)]
    if missing:
        print(f"FAIL: missing mtools utilities: {', '.join(missing)}",
              file=sys.stderr)
        return 1

    try:
        with tempfile.TemporaryDirectory(prefix="aescos-fat-test-") as temp:
            image = Path(temp) / "fat12.img"
            create_fat12_image(image, Path(arguments.init),
                               Path(arguments.bin_dir))
            run_test(arguments.qemu, arguments.kernel, arguments.timeout,
                     str(image))

            expected = (Path(arguments.bin_dir) / "ver").read_bytes()
            copied = subprocess.run(
                ["mtype", "-i", str(image), "::/PERSIST.BIN"],
                check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            ).stdout
            require(copied == expected,
                    "persisted file contents differ from the source ELF")

            test = ShellSmokeTest(arguments.qemu, arguments.kernel,
                                  arguments.timeout, str(image))
            try:
                test.boot()
                output = test.command("dir")
                require(b"PERSIST.BIN" in output,
                        "copied file was missing after reboot")
            finally:
                test.close()
            print("PASS: FAT contents persist after reboot")
    except (AssertionError, OSError, RuntimeError, TimeoutError,
            subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", None)
        if detail:
            error = f"{error}: {detail.decode(errors='replace')}"
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
