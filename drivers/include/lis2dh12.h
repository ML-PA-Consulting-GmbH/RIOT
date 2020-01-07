/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_lis2dh12 LIS2DH12 Accelerometer
 * @ingroup     drivers_sensors
 * @ingroup     drivers_saul
 * @brief       Driver for the STM LIS2DH12 accelerometer
 *
 * This device driver provides a minimal interface to LIS2DH12 devices. As of
 * now, it only provides very basic access to the device. The driver configures
 * the device to continuously read the acceleration data with statically
 * defined scale and rate, and with a fixed 10-bit resolution. The LIS2DH12's
 * FIFO is bypassed, so the driver might not be sufficient for use cases where
 * the complete history of readings is of interest.
 *
 * Also, the current version of the driver supports only interfacing the sensor
 * via SPI. The driver is however written in a way, that adding I2C interface
 * support is quite simple, as all bus related functions (acquire, release,
 * read, write) are cleanly separated in the code.
 *
 * This driver provides @ref drivers_saul capabilities.
 *
 * @{
 * @file
 * @brief       Interface definition for the STM LIS2DH12 accelerometer
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef LIS2DH12_H
#define LIS2DH12_H

#include <stdint.h>

#include "saul.h"

#ifdef MODULE_LIS2DH12_SPI
#include "periph/spi.h"
#include "periph/gpio.h"
#else
#include "periph/i2c.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MODULE_LIS2DH12) || defined(DOXYGEN)
/**
 * @brief   Default I2C slave address for LIS2DH12 devices
 */
#define LIS2DH12_ADDR_DEFAULT       (0x19)
#endif

/**
 * @brief   Available scale values
 */
typedef enum {
    LIS2DH12_SCALE_2G  = 0x00,      /**< +- 2g */
    LIS2DH12_SCALE_4G  = 0x10,      /**< +- 4g */
    LIS2DH12_SCALE_8G  = 0x20,      /**< +- 8g */
    LIS2DH12_SCALE_16G = 0x30,      /**< +- 16g */
} lis2dh12_scale_t;

/**
 * @brief   Available sampling rates
 *
 * @note    The device does also support some additional rates for specific low-
 *          power modes, but those are as of now not supported by this driver
 */
typedef enum {
    LIS2DH12_RATE_1HZ   = 0x17,     /**< sample with 1Hz */
    LIS2DH12_RATE_10HZ  = 0x27,     /**< sample with 10Hz */
    LIS2DH12_RATE_25HZ  = 0x37,     /**< sample with 25Hz */
    LIS2DH12_RATE_50HZ  = 0x47,     /**< sample with 50Hz */
    LIS2DH12_RATE_100HZ = 0x57,     /**< sample with 100Hz */
    LIS2DH12_RATE_200HZ = 0x67,     /**< sample with 200Hz */
    LIS2DH12_RATE_400HZ = 0x77,     /**< sample with 400Hz */
} lis2dh12_rate_t;

/**
 * @brief   LIS2DH12 configuration parameters
 */
typedef struct {
#ifdef MODULE_LIS2DH12_SPI
    spi_t spi;                      /**< SPI bus the device is connected to */
    gpio_t cs;                      /**< connected chip select pin */
#else
    i2c_t i2c;                      /**< I2C bus the device is connected to */
    uint8_t addr;                   /**< device address on the I2C bus */
#endif
    lis2dh12_scale_t scale;         /**< sampling sensitivity used */
    lis2dh12_rate_t rate;           /**< sampling rate used */
} lis2dh12_params_t;

/**
 * @brief   LIS2DH12 device descriptor
 */
typedef struct {
    const lis2dh12_params_t *p;     /**< device configuration */
    uint16_t comp;                  /**< scale compensation factor */
} lis2dh12_t;

/**
 * @brief   Status and error return codes
 */
enum {
    LIS2DH12_OK    =  0,            /**< everything was fine */
    LIS2DH12_NOBUS = -1,            /**< bus interface error */
    LIS2DH12_NODEV = -2,            /**< unable to talk to device */
    LIS2DH12_NOINT = -3,            /**< wrong interrupt line was given (1 or 2) */
};

/**
 * @brief    Parameter for interrupt configuration
 */
typedef struct {
    uint8_t int_config;             /**< values for configuration */
    uint8_t int_threshold:7;        /**< the threshold for triggering interrupt, threshold in range 0-127 */
    uint8_t int_duration:7;         /**< time between two interrupts, calc from <duration>/ODR, ODR section in CTRL_REG1, duration in range 0-127 */
    uint8_t int_type;               /**< values for type of interrupts */
} lis2dh12_int_params_t;

/**
 * @brief   Status of INT_SRC register
 */
typedef struct {
    uint8_t LIS2DH12_INT_SRC_XL:1;  /**< X low event has occured */
    uint8_t LIS2DH12_INT_SRC_XH:1;  /**< X high event has occured */
    uint8_t LIS2DH12_INT_SRC_YL:1;  /**< Y low event has occured */
    uint8_t LIS2DH12_INT_SRC_YH:1;  /**< Y high event has occured */
    uint8_t LIS2DH12_INT_SRC_ZL:1;  /**< Z low event has occured */
    uint8_t LIS2DH12_INT_SRC_ZH:1;  /**< Z high event has occured */
    uint8_t LIS2DH12_INT_SRC_IA:1;  /**< 1 if interrupt occured */
} lis2dh12_int_src_reg_t;

/**
 * @brief   Export the SAUL interface for this driver
 */
extern const saul_driver_t lis2dh12_saul_driver;

/**
 * @brief   Set the interrupt values in LIS2DH12 sensor device
 *
 * @param[in] dev      device descriptor
 * @param[in] params   device interrupt configuration, with .type, .cfg, .ths and .duration
 * @param[in] int_line number of interrupt line (1 or 2)
 *
 * @return  LIS2DH12_OK on success
 * @return  LIS2DH12_NOBUS on bus errors
 */
int lis2dh12_set_int(const lis2dh12_t *dev, lis2dh12_int_params_t params, uint8_t int_line);

/**
 * @brief   Read an interrupt event on LIS2DH12 sensor device
 *
 * @param[in] dev      device descriptor
 * @param[out] data    device interrupt data
 * @param[in] int_line number of interrupt line (1 or 2)
 *
 * @return  LIS2DH12_OK on success
 * @return  LIS2DH12_NOBUS on bus errors
 */
int lis2dh12_read_int_src(const lis2dh12_t *dev, lis2dh12_int_src_reg_t *data, uint8_t int_line);

/**
 * @brief   Initialize the given LIS2DH12 sensor device
 *
 * @param[out] dev      device descriptor
 * @param[in]  params   static device configuration
 *
 * @return  LIS2DH12_OK on success
 * @return  LIS2DH12_NOBUS on bus errors
 * @return  LIS2DH12_NODEV if no LIS2DH12 device was found on the bus
 */
int lis2dh12_init(lis2dh12_t *dev, const lis2dh12_params_t *params);

/**
 * @brief   Read acceleration data from the given device
 *
 * @param[in]  dev      device descriptor
 * @param[out] data     acceleration data in mili-g, **MUST** hold 3 values
 *
 * @return  LIS2DH12_OK on success
 * @return  LIS2DH12_NOBUS on bus error
 */
int lis2dh12_read(const lis2dh12_t *dev, int16_t *data);

/**
 * @brief   Power on the given device
 *
 * @param[in] dev       device descriptor
 *
 * @return  LIS2DH12_OK on success
 * @return  LIS2DH12_NOBUS on bus error
 */
int lis2dh12_poweron(const lis2dh12_t *dev);

/**
 * @brief   Power off the given device
 *
 * @param[in] dev       device descriptor
 *
 * @return  LIS2DH12_OK on success
 * @return  LIS2DH12_NOBUS on bus error
 */
int lis2dh12_poweroff(const lis2dh12_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* LIS2DH12_H */
/** @} */
