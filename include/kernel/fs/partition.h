#pragma once

#include "kernel/fs/blockdev.h"

/* Scan the primary MBR entries on hd0; returns the registered partition count. */
int partition_scan(struct blockdev *disk);
