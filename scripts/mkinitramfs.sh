#!/bin/bash
set -e

echo "[*] Building initramfs..."

rm -f initramfs.tar

# IMPORTANT: go inside base so paths are correct (/bin, /etc, etc)
cd base
tar -cf ../initramfs.tar .

cd ..

echo "[+] initramfs.tar created"