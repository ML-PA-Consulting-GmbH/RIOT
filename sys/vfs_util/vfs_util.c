/*
 * Copyright (C) 2021 Benjamin Valentin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_vfs_util
 * @{
 * @file
 * @brief   VFS layer helper functions
 * @author  Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#include <fcntl.h>
#include <string.h>

#include "vfs.h"
#include "vfs_util.h"

#define ENABLE_DEBUG 0
#include "debug.h"

int vfs_file_from_buffer(const char *file, const void *buf, size_t len)
{
    int res, fd = vfs_open(file, O_CREAT | O_TRUNC | O_WRONLY, 0644);

    if (fd < 0) {
        DEBUG("can't open %s for writing\n", file);
        return fd;
    }

    res = vfs_write(fd, buf, len);
    vfs_close(fd);

    if (res) {
        return res;
    }

    return 0;
}

int vfs_file_to_buffer(const char* file, void* buf, size_t len)
{
    int res, fd = vfs_open(file, O_RDONLY, 0);

    if (fd < 0) {
        DEBUG("can't open %s for reading\n", file);
        return fd;
    }

    res = vfs_read(fd, buf, len);
    vfs_close(fd);

    if (res > 0) {
        memset((char *)buf + res, 0, len - res);
    }

    return res;
}
