/*
 * Copyright (C) 2220 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_mtd_spi_eeprom mtd wrapper for spi_eeprom like at25xxx or M95xxx
 * @ingroup     drivers_storage
 * @brief       Driver for SPI-EEPROMs using mtd interface
 *
 * @{
 *
 * @file
 * @brief       Interface definition for mtd_spi_eeprom driver
 *
 * @author      Johannes Koster <johannes.koster@ml-pa.com>
 */

#ifndef MTD_SPI_EEPROM_H
#define MTD_SPI_EEPROM_H

#include <stdint.h>

#include "at25xxx.h"
#include "mtd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   Device descriptor for mtd_spi_eeprom device
 *
 * This is an extension of the @c mtd_dev_t struct
 */
typedef struct {
    mtd_dev_t base;                    /**< inherit from mtd_dev_t object */
    at25xxx_t *spi_eeprom;             /**< spi_eeprom dev descriptor */
    const at25xxx_params_t *params; /**< params for spi_eeprom init */
} mtd_spi_eeprom_t;


/**
 * @brief   EEPROMs handle sector erase internally so it's possible to directly
 *          write to the EEPROM without erasing the sector first.
 *          Attention: an erase call will therefore NOT touch the content,
 *                     so disable this feature to ensure overriding the data.
 */
#ifndef MTD_SPI_EEPROM_SKIP_ERASE
#define MTD_SPI_EEPROM_SKIP_ERASE (1)
#endif

/**
 * @brief   spi_eeprom device operations table for mtd
 */
extern const mtd_desc_t mtd_spi_eeprom_driver;

#ifdef __cplusplus
}
#endif

#endif /* MTD_SPI_EEPROM_H */
/** @} */
