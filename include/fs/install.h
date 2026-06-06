/**
 * @file install.h
 * @brief Disk install and bootstrap utilities.
 *
 * Provides functions for:
 * - Detecting and mounting disk root filesystem
 * - Checking if a disk installation already exists
 * - Creating new partitions and filesystems
 */

#pragma once

#include "libk/types.h"

/**
 * @brief Try to detect and mount disk root filesystem.
 *
 * Attempts to mount an existing filesystem on the primary ATA device
 * as the root filesystem. Tries FAT first, then EXT2.
 *
 * @return 0 if disk root successfully mounted, non-zero otherwise.
 */
int install_mount_disk_root(void);

/**
 * @brief Check if disk already has a valid installation.
 *
 * Checks if the primary ATA device has a filesystem with an /init file.
 *
 * @return 1 if disk has valid installation, 0 otherwise.
 */
int install_check_disk_installed(void);

/**
 * @brief Format disk with FAT32 filesystem and copy root files.
 *
 * Creates a minimal bootable filesystem on the primary ATA device.
 * This is called by the installer process.
 *
 * @return 0 on success, non-zero on failure.
 */
int install_format_and_copy(void);
