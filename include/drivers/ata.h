/**
 * @file ata.h
 * @brief ATA PIO driver interface and register definitions.
 *
 * This header defines the I/O port layout, status flags, and public
 * functions for interacting with ATA devices in PIO mode. The driver
 * supports IDENTIFY, 28‑bit LBA reads, and 28‑bit LBA writes.
 */

#pragma once

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* I/O Port Base Addresses                                                     */
/* -------------------------------------------------------------------------- */

/** @brief Base I/O port for the primary ATA bus. */
#define ATA_PRIMARY_IO   0x1F0

/** @brief Control port for the primary ATA bus. */
#define ATA_PRIMARY_CTRL 0x3F6

/* -------------------------------------------------------------------------- */
/* ATA Register Offsets                                                        */
/* -------------------------------------------------------------------------- */

#define ATA_REG_DATA      0x00  /**< Data register (R/W). */
#define ATA_REG_ERROR     0x01  /**< Error register (R). */
#define ATA_REG_SECCOUNT  0x02  /**< Sector count. */
#define ATA_REG_LBA0      0x03  /**< LBA low byte. */
#define ATA_REG_LBA1      0x04  /**< LBA mid byte. */
#define ATA_REG_LBA2      0x05  /**< LBA high byte. */
#define ATA_REG_HDDEVSEL  0x06  /**< Drive/head select. */
#define ATA_REG_COMMAND   0x07  /**< Command register (W). */
#define ATA_REG_STATUS    0x07  /**< Status register (R). */

/* -------------------------------------------------------------------------- */
/* ATA Commands                                                                */
/* -------------------------------------------------------------------------- */

#define ATA_CMD_IDENTIFY  0xEC  /**< IDENTIFY DEVICE command. */

/* -------------------------------------------------------------------------- */
/* ATA Status Register Flags                                                   */
/* -------------------------------------------------------------------------- */

#define ATA_SR_BSY  0x80  /**< Controller is busy. */
#define ATA_SR_DRDY 0x40  /**< Device is ready. */
#define ATA_SR_DRQ  0x08  /**< Data request ready. */
#define ATA_SR_ERR  0x01  /**< Error occurred. */

/* -------------------------------------------------------------------------- */
/* Public Driver API                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Issue an ATA IDENTIFY command to detect the primary drive.
 *
 * Reads the IDENTIFY data block (512 bytes) and validates that a drive
 * is present and responding.
 *
 * @return 0 on success, negative error code on failure.
 */
int ata_identify(void);

/**
 * @brief Read sectors using 28‑bit LBA addressing.
 *
 * Performs a PIO read of @p count sectors starting at @p lba into @p buf.
 * The buffer must be large enough to hold `count * 512` bytes.
 *
 * @param lba   Starting 28‑bit LBA address.
 * @param buf   Destination buffer.
 * @param count Number of sectors to read.
 * @return 0 on success, negative error code on failure.
 */
int ata_read28(u32 lba, void *buf, u32 count);

/**
 * @brief Write sectors using 28‑bit LBA addressing.
 *
 * Performs a PIO write of @p count sectors from @p buf to @p lba.
 *
 * @param lba   Starting 28‑bit LBA address.
 * @param buf   Source buffer.
 * @param count Number of sectors to write.
 * @return 0 on success, negative error code on failure.
 */
int ata_write28(u32 lba, const void *buf, u32 count);

