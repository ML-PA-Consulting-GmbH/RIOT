/**
 * @ingroup
 * @{
 *
 * @file
 * @brief       Default parameters for spi_nor_flash
 *
 * @author      Jan Mohr <jan.mohr@ml-pa.com>
 */

#ifndef SPI_NOR_PARAMS_H
#define SPI_NOR_PARAMS_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters for the spi_nor driver
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

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_PARAMS_H */
/** @} */
