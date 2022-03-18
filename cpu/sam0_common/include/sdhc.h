/*
 * Copyright (C) 2018 Alkgrove
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    cpu_sam0_common_sdhc sam0 SD Host Controller
 * @ingroup     cpu_sam0_common
 * @brief       SD card interface functions for sam0 class devices
 *
 * @{
 *
 * @file
 * @brief       SD card interface functions for sam0 class devices
 *
 * @author      alkgrove <bob@alkgrove.com>
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#ifndef SDHC_H
#define SDHC_H

#include <stdint.h>
#include <stdbool.h>
#include "periph/gpio.h"
#include "mutex.h"

/** @name Card State */
/** @{ */
typedef struct {
    Sdhc *dev;              /**< SDHC instance              */
    gpio_t cd;              /**< Card detect pin            */
    gpio_t wp;              /**< Write Protect pin          */
    mutex_t sync;           /**< ISR mutex                  */
    uint32_t sectors;       /**< Capacity in bytes          */
    uint32_t clock;         /**< Accepted Cloc Rate in Hz   */
    uint16_t rca;           /**< Relative Card Address      */
    uint16_t error;         /**< Last error state           */
    uint8_t type;           /**< Type of Card               */
    uint8_t version;        /**< Version of Card            */
    uint8_t bus_width;      /**< Acceptable Bus Width (1 or 4) */
    bool high_speed;        /**< Turbo mode                 */
    bool need_init;         /**< Card installed but not initialized if true */
} sdhc_state_t;
/** @} */

/** @name Card Types */
/** @{ */
#define CARD_TYPE_UNKNOWN   (0)         /**< Unknown type card */
#define CARD_TYPE_SD        (1 << 0)    /**< SD card */
#define CARD_TYPE_MMC       (1 << 1)    /**< MMC card */
#define CARD_TYPE_SDIO      (1 << 2)    /**< SDIO card */
#define CARD_TYPE_HC        (1 << 3)    /**< High capacity card */
/** SD combo card (io + memory) */
#define CARD_TYPE_SD_COMBO (CARD_TYPE_SD | CARD_TYPE_SDIO)
/** @} */

/** @name Card Versions */
/** @{ */
#define CARD_VER_UNKNOWN    (0)     /**< Unknown card version */
#define CARD_VER_SD_1_0     (0x10)  /**< SD version 1.0 and 1.01 */
#define CARD_VER_SD_1_10    (0x1A)  /**< SD version 1.10 */
#define CARD_VER_SD_2_0     (0X20)  /**< SD version 2.00 */
#define CARD_VER_SD_3_0     (0X30)  /**< SD version 3.0X */
#define CARD_VER_MMC_1_2    (0x12)  /**< MMC version 1.2 */
#define CARD_VER_MMC_1_4    (0x14)  /**< MMC version 1.4 */
#define CARD_VER_MMC_2_2    (0x22)  /**< MMC version 2.2 */
#define CARD_VER_MMC_3      (0x30)  /**< MMC version 3 */
#define CARD_VER_MMC_4      (0x40)  /**< MMC version 4 */
/** @} */

/** @name Flags used to define MCI parser for SD/MMC command */
/** @{ */
#define MCI_RESP_PRESENT    (1ul <<  8)     /**< Have response */
#define MCI_RESP_136        (1ul << 11)     /**< 136 bit response */
#define MCI_RESP_CRC        (1ul << 12)     /**< Expect valid crc */
#define MCI_RESP_BUSY       (1ul << 13)     /**< Card may send busy */
#define MCI_CMD_OPENDRAIN   (1ul << 14)     /**< Open drain for a braodcast command */
#define MCI_CMD_WRITE       (1ul << 15)     /**< To signal a data write operation */
#define MCI_CMD_SDIO_BYTE   (1ul << 16)     /**< To signal a SDIO tranfer in multi byte mode */
#define MCI_CMD_SDIO_BLOCK  (1ul << 17)     /**< To signal a SDIO tranfer in block mode */
#define MCI_CMD_STREAM      (1ul << 18)     /**< To signal a data transfer in stream mode */
#define MCI_CMD_SINGLE_BLOCK (1ul << 19)    /**< To signal a data transfer in single block mode */
#define MCI_CMD_MULTI_BLOCK (1ul << 20)     /**< To signal a data transfer in multi block mode */
/** @} */

/** This SD stack uses the maximum block size authorized (512 bytes) */
#define SD_MMC_BLOCK_SIZE   512
#define SDHC_SLOW_CLOCK_HZ  400000

/** @name Error Codes */
/** @{ */
#define SDHC_OK                     0
#define SDHC_ERR_CARD_NOT_PRESENT   1
#define SDHC_ERR_BAD_CARD           2
#define SDHC_ERR_SDIO_NOT_SUPPORTED 3
#define SDHC_ERR_BUSY               4
/** @} */

/**
 * @brief
 *
 * @param[in] state - sdhc_state_t *
 * @return return error code
 */
int sdhc_init(sdhc_state_t *state);

/**
 * @brief
 *
 * @param[in] cmd - uint32_t
 * @param[in] arg - uint32_t
 * @return true if success, false if failed
 */
bool sdhc_send_cmd(sdhc_state_t *state, uint32_t cmd, uint32_t arg);

/**
 * @brief sdhc_read_blocks
 * read 512 byte blocks for block_count blocks from SD/MMC card with address
 * to filebuffer pointed to by dst
 * @param[in] state - sdhc_state_t *
 * @param[in] address - uint32_t
 * @param[in] dst - uint8_t *
 * @return true if success, false if failed
 */
int sdhc_read_block(sdhc_state_t *state, uint32_t address, void *dst);

/**
 * @brief sdhc_read_blocks
 * read n 512 byte blocks for block_count blocks from SD/MMC card with address
 * to filebuffer pointed to by dst
 * @param[in] state - sdhc_state_t *
 * @param[in] address - uint32_t
 * @param[in] dst - uint8_t *
 * @param[in] num_blocks - uint16_t number of blocks to read
 * @return true if success, false if failed
 */
int sdhc_read_blocks(sdhc_state_t *state, uint32_t address, void *dst, uint16_t num_blocks);

/**
 * @brief sdhc_write_block
 * writes a 512 byte block file buffer pointed to by src to SD/MMC card with address
 * for block_count blocks
 * @param[in] state - sdhc_state_t *
 * @param[in] address - uint32_t  disc byte (or sector) address
 * @param[in] src - uint8_t * pointer to memory to write
 * @return true if success, false if failed
 */
int sdhc_write_block(sdhc_state_t *state, uint32_t address, const void *src);

/**
 * @brief sdhc_write_blocks
 * writes n 512 byte blocks file buffer pointed to by src to SD/MMC card with address
 * for block_count blocks
 * @param[in] state - sdhc_state_t *
 * @param[in] address - uint32_t  disc byte (or sector) address
 * @param[in] src - uint8_t * pointer to memory to write
 * @param[in] num_blocks - uint16_t number of blocks to write
 * @return true if success, false if failed
 */
int sdhc_write_blocks(sdhc_state_t *state, uint32_t address, const void *src,
                      uint16_t num_blocks);

/**
 * @brief sdhc_erase_blocks
 * erases n 512 byte blocks
 * @param[in] state - sdhc_state_t *
 * @param[in] start - uint32_t  disc byte (or sector) address
 * @param[in] num_blocks - uint16_t number of blocks to write
 * @return true if success, false if failed
 */
int sdhc_erase_blocks(sdhc_state_t *state, uint32_t start, uint16_t num_blocks);

#endif /* SDHC_H */
