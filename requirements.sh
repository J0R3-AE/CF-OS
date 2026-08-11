#!/bin/bash

sudo pacman -Syu --noconfirm

# Build tools
sudo pacman -S --noconfirm base-devel make bison flex texinfo

# GRUB + ISO tools
sudo pacman -S --noconfirm grub xorriso mtools

# Assembly tools
sudo pacman -S --noconfirm nasm

# QEMU + virtualization
sudo pacman -S --noconfirm qemu-full virt-manager bridge-utils

# Multilib + cross‑compilers
sudo pacman -S --noconfirm gcc-multilib lib32-glibc
sudo pacman -S --noconfirm gcc binutils

# GMP / MPC / MPFR (compiler math libs)
sudo pacman -S --noconfirm gmp libmpc mpfr

echo "All packages installed successfully."
