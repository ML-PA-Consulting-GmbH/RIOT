/*
 * Copyright (C) 2017 Michel Rottleuthner <michel.rottleuthner@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_sdcard_spi
 * @{
 *
 * @file
 * @brief       Default parameters for sdcard_spi driver
 *
 * @author      Michel Rottleuthner <michel.rottleuthner@haw-hamburg.de>
 */

#ifndef SDCARD_SPI_PARAMS_H
#define SDCARD_SPI_PARAMS_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters for the sdcard_spi driver
 * @{
 */
#ifndef SDCARD_SPI_PARAM_SPI
#define SDCARD_SPI_PARAM_SPI         SPI_DEV(0)
#endif
#ifndef SDCARD_SPI_PARAM_CS
#define SDCARD_SPI_PARAM_CS          GPIO_PIN(2,4)
#endif
#ifndef SDCARD_SPI_PARAM_CLK
#define SDCARD_SPI_PARAM_CLK         GPIO_PIN(2,5)
#endif
#ifndef SDCARD_SPI_PARAM_MOSI
#define SDCARD_SPI_PARAM_MOSI        GPIO_PIN(2,6)
#endif
#ifndef SDCARD_SPI_PARAM_MISO
#define SDCARD_SPI_PARAM_MISO        GPIO_PIN(2,7)
#endif
#ifndef SDCARD_SPI_PARAM_POWER
#define SDCARD_SPI_PARAM_POWER       (GPIO_UNDEF)
#endif
#ifndef SDCARD_SPI_PARAM_POWER_AH
/** treated as 'don't care' if SDCARD_SPI_PARAM_POWER is GPIO_UNDEF */
#define SDCARD_SPI_PARAM_POWER_AH    (true)
#endif

#ifndef SDCARD_SPI_PARAMS
#define SDCARD_SPI_PARAMS            { .spi_dev   = SDCARD_SPI_PARAM_SPI, \
                                       .cs    = SDCARD_SPI_PARAM_CS,      \
                                       .clk   = SDCARD_SPI_PARAM_CLK,     \
                                       .mosi  = SDCARD_SPI_PARAM_MOSI,    \
                                       .miso  = SDCARD_SPI_PARAM_MISO,    \
                                       .power = SDCARD_SPI_PARAM_POWER,   \
                                       .power_act_high = SDCARD_SPI_PARAM_POWER_AH }
#endif
/** @} */

/**
 * @brief   sdcard_spi configuration
 */
static const  sdcard_spi_params_t sdcard_spi_params[] = {
    SDCARD_SPI_PARAMS
};

/**
 * @name    Set retry parameters for specific actions
 *
 * Retry counters as timeouts or timeouts in microseconds for specific actions.
 * The values may need some adjustments to either give the card more time to respond
 * to commands or to achieve a lower delay / avoid infinite blocking.
 * <p>
 * Interpretation: 0 means execute once, positive values provide a maximum retry count,
 * negative values provide a number of microseconds until the timeout gets effective.
 * The value range is 32 bit signed integer.
 *
 * @{
 */
#ifndef INIT_CMD_RETRY
#define INIT_CMD_RETRY           (1000000) /**< initialization command retry */
#endif
#ifndef INIT_CMD0_RETRY
#define INIT_CMD0_RETRY          (3)       /**< initialization command 0 retry */
#endif
#ifndef R1_POLLING_RETRY
#define R1_POLLING_RETRY         (1000000) /**< initialization first response */
#endif
#ifndef SD_DATA_TOKEN_RETRY
#define SD_DATA_TOKEN_RETRY      (1000000) /**< data packet token read retry */
#endif
#ifndef SD_WAIT_FOR_NOT_BUSY
#define SD_WAIT_FOR_NOT_BUSY     (1000000) /**< wait for SD card */
#endif
#ifndef SD_BLOCK_READ_CMD_RETRY
#define SD_BLOCK_READ_CMD_RETRY  (10)      /**< only affects sending of cmd not whole transaction! */
#endif
#ifndef SD_BLOCK_WRITE_CMD_RETRY
#define SD_BLOCK_WRITE_CMD_RETRY (10)      /**< only affects sending of cmd not whole transaction! */
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SDCARD_SPI_PARAMS_H */
/** @} */
