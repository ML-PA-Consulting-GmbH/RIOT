/*
 * Copyright (C) 2220 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/**
 * @ingroup     drivers_mtd_spi_eeprom
 * @{
 *
 * @file
 * @brief       Driver for using spi eeprom (like AT25xxx, M95xxx, 25AAxxx, 25LCxxx,
 *              CAT25xxx & BR25Sxxx) as mtd
 *
 * @author      Johannes Koster <johannes.koster@ml-pa.com>
 *
 * @}
 */
#define ENABLE_DEBUG (0)
#include "debug.h"
#include "mtd.h"
#include "mtd_spi_eeprom.h"
#include "at25xxx.h"
#include "at25xxx_params.h"

#include <inttypes.h>
#include <errno.h>

static int mtd_spi_eeprom_init(mtd_dev_t *mtd);
static int mtd_spi_eeprom_read(mtd_dev_t *mtd, void *dest, uint32_t addr,
                           uint32_t size);
static int mtd_spi_eeprom_write(mtd_dev_t *mtd, const void *src, uint32_t addr,
                            uint32_t size);
static int mtd_spi_eeprom_erase(mtd_dev_t *mtd, uint32_t addr, uint32_t size);
static int mtd_spi_eeprom_power(mtd_dev_t *mtd, enum mtd_power_state power);

const mtd_desc_t mtd_spi_eeprom_driver = {
    .init = mtd_spi_eeprom_init,
    .read = mtd_spi_eeprom_read,
    .write = mtd_spi_eeprom_write,
    .erase = mtd_spi_eeprom_erase,
    .power = mtd_spi_eeprom_power,
};

static int mtd_spi_eeprom_init(mtd_dev_t *dev)
{
    DEBUG("mtd_spi_eeprom_init\n");
    mtd_spi_eeprom_t *mtd_eeprom = (mtd_spi_eeprom_t*)dev;
    if (at25xxx_init(mtd_eeprom->spi_eeprom, mtd_eeprom->params) == 0)
    {
        dev->pages_per_sector = 1;
        dev->page_size        = AT25XXX_PARAM_PAGE_SIZE;
        return 0;
    }
    return -EIO;
}

static int mtd_spi_eeprom_read(mtd_dev_t *dev, void *buff, uint32_t addr,
                           uint32_t size)
{
    DEBUG("mtd_eeprom_read: addr:%" PRIu32 " size:%" PRIu32 "\n", addr, size);
    mtd_spi_eeprom_t *mtd_eeprom = (mtd_spi_eeprom_t*)dev;
    size_t res = at25xxx_read(mtd_eeprom->spi_eeprom, addr, buff, size);
    if (res == size) {
        return res;
    }
    return -EIO;
}

static int mtd_spi_eeprom_write(mtd_dev_t *dev, const void *buff, uint32_t addr,
                            uint32_t size)
{
    DEBUG("mtd_eeprom_write: addr:%" PRIu32 " size:%" PRIu32 "\n", addr, size);
    mtd_spi_eeprom_t *mtd_eeprom = (mtd_spi_eeprom_t*)dev;
    size_t res = at25xxx_write(mtd_eeprom->spi_eeprom, addr,
                                    buff, size);
    if (res) {
        return res;
    }
    return -EIO;
}

static int mtd_spi_eeprom_erase(mtd_dev_t *dev,
                            uint32_t addr,
                            uint32_t size)
{
    DEBUG("mtd_spi_eeprom_erase: addr:%" PRIu32 " size:%" PRIu32 "\n", addr, size);
    mtd_spi_eeprom_t *mtd_eeprom = (mtd_spi_eeprom_t*)dev;
    size_t res = at25xxx_clear(mtd_eeprom->spi_eeprom, addr, size);
    if (res == size) {
        return 0;
    }
    return -ENOTSUP;
}

static int mtd_spi_eeprom_power(mtd_dev_t *dev, enum mtd_power_state power)
{
    (void)dev;
    (void)power;

    /* TODO: implement power down of EEPROM (in at25xxx driver?)
    */
    return -ENOTSUP; /* currently not supported */
}
