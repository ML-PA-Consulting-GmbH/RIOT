/*
 * Copyright (C) 2023 ML!PA Consulting Gmbh
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     sys_configuration
 * @{
 *
 * @file
 * @brief       Implementation of the FlashDB configuration backend
 *
 * @author      Fabian Hüßler <fabian.huessler@ml-pa.com>
 *
 * @}
 */

#include <assert.h>
#include <errno.h>

#include "configuration.h"
#include "configuration_backend_flashdb.h"
#include "mutex.h"
#if IS_USED(MODULE_CONFIGURATION_BACKEND_FLASHDB_VFS)
#include "vfs_default.h"
#endif

static mutex_t _kvdb_locker = MUTEX_INIT;
static struct fdb_kvdb _kvdb = { 0 };

static void _lock(fdb_db_t db)
{
    mutex_lock(db->user_data);
}

static void _unlock(fdb_db_t db)
{
    mutex_unlock(db->user_data);
}

static int _be_fdb_init(mtd_dev_t *mtd)
{
    assert(mtd);
    uint32_t size;
#if IS_USED(MODULE_CONFIGURATION_BACKEND_FLASHDB_VFS)
    bool file_mode = true;
    fdb_kvdb_control(&_kvdb, FDB_KVDB_CTRL_SET_FILE_MODE, &file_mode);
    int fail;
    if ((fail = vfs_mkdir(CONFIGURATION_FLASHDB_VFS_PATH, 0777)) < 0) {
        if (fail != -EEXIST) {
            /* probably not mounted (try with vfs_auto_format) */
            return fail;
        }
    }
#endif
    /* The MTD must be initialized! */
    size = mtd->pages_per_sector * mtd->page_size;
    size = ((size - 1) + (1u << CONFIGURATION_FLASHDB_MIN_SECTOR_SIZE_EXP))
           / size * size;
    /* sector size must be set before max size */
    fdb_kvdb_control(&_kvdb, FDB_KVDB_CTRL_SET_SEC_SIZE, &size);
#if IS_USED(MODULE_CONFIGURATION_BACKEND_FLASHDB_VFS)
    size *= CONFIGURATION_FLASHDB_VFS_MAX_SECTORS;
    fdb_kvdb_control(&_kvdb, FDB_KVDB_CTRL_SET_MAX_SIZE, &size);
#endif
    fdb_kvdb_control(&_kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)(uintptr_t)_lock);
    fdb_kvdb_control(&_kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)(uintptr_t)_unlock);
    if (fdb_kvdb_init(&_kvdb,
                      "kvdb_configuration",
#if IS_USED(MODULE_CONFIGURATION_BACKEND_FLASHDB_VFS)
                      CONFIGURATION_FLASHDB_VFS_PATH,
#elif IS_USED(MODULE_CONFIGURATION_BACKEND_FLASHDB_MTD)
                      CONFIGURATION_FLASHDB_MTD_PARTITION_LABEL,
#endif
                      NULL,
                      &_kvdb_locker) != FDB_NO_ERR) {
        return -EINVAL;
    }
    return 0;
}

static int _be_fdb_reset(void) {
    fdb_err_t result;
    if ((result = fdb_kv_set_default(&_kvdb)) != FDB_NO_ERR) {
        return -EIO;
    }
    return 0;
}

static int _be_fdb_load(const struct conf_backend *be,
                        const char *key, void *val, size_t *size)
{
    (void)be;
    struct fdb_blob blob;
    size_t sz;
    if ((sz = fdb_kv_get_blob(&_kvdb, key, fdb_blob_make(&blob, val, *size))) <= 0) {
        return -EIO;
    }
    if (!blob.saved.len) {
        return -ENODATA;
    }
    return 0;

}

static int _be_fdb_store(const struct conf_backend *be,
                         const char *key, const void *val, size_t *size,
                         off_t part_offset, size_t part_size)
{
    /* Flash cannot take real benefit from offset parameter */
    (void)be; (void)part_offset; (void)part_size;
    struct fdb_blob blob;
    fdb_err_t err;
    if ((err = fdb_kv_set_blob(&_kvdb, key, fdb_blob_make(&blob, val, *size))) != FDB_NO_ERR) {
        return -EIO;
    }
    if (!blob.saved.len) {
        return -ENODATA;
    }
    return 0;
}

static int _be_fdb_delete(const struct conf_backend *be, const char *key)
{
    (void)be;
    fdb_err_t err;
    /* not an error if key does not exist */
    if ((err = fdb_kv_del(&_kvdb, key)) != FDB_NO_ERR && err != FDB_KV_NAME_ERR) {
        return -EIO;
    }
    return 0;
}

static const conf_backend_ops_t _be_fdb_ops = {
    .be_load = _be_fdb_load,
    .be_store = _be_fdb_store,
    .be_delete = _be_fdb_delete,
};

static const conf_backend_t _be_fdb =  {
    .ops = &_be_fdb_ops,
};

const conf_backend_t *configuration_backend_flashdb_get(void)
{
    return &_be_fdb;
}

int configuration_backend_flashdb_reset(void)
{
    return _be_fdb_reset();
}

int configuration_backend_flashdb_init(mtd_dev_t *mtd)
{
    return _be_fdb_init(mtd);
}

void auto_init_configuration_backend_flashdb(void)
{
#if IS_USED(MODULE_CONFIGURATION_BACKEND_FLASHDB_MTD)
    extern void fdb_mtd_init(mtd_dev_t *mtd);
    fdb_mtd_init(CONFIGURATION_FLASHDB_MTD_DEV);
    int fail = configuration_backend_flashdb_init(CONFIGURATION_FLASHDB_MTD_DEV);
#else
    int fail = configuration_backend_flashdb_init(CONFIGURATION_FLASHDB_VFS_MTD_DEV);
#endif
    assert(!fail); (void)fail;
#if (IS_USED(MODULE_CONFIGURATION_BACKEND_RESET_FLASHDB))
    fail = configuration_backend_flashdb_reset();
    assert(!fail);
#endif
}

#ifndef AUTO_INIT_PRIO_MOD_CONFIGURATION_BACKEND_FLASHDB
#define AUTO_INIT_PRIO_MOD_CONFIGURATION_BACKEND_FLASHDB    1010
#endif

AUTO_INIT_CONFIGURATION(auto_init_configuration_backend_flashdb,
                        AUTO_INIT_PRIO_MOD_CONFIGURATION_BACKEND_FLASHDB);
