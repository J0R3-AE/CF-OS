#!/bin/bash

echo MULTIBOOT CHECK:---------------------------------------------
readelf -S build/kernel.elf | grep multiboot
echo HEX CHECK:---------------------------------------------------
hexdump -h build/kernel.elf | grep 1badb002
echo ELF ARCH CHECK:----------------------------------------------
readelf -h build/kernel.elf
