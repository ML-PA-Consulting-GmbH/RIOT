/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_sam0_common
 * @ingroup     drivers_periph_adc
 * @{
 *
 * @file
 * @brief       Low-level flash page driver implementation
 *
 * The sam0 has its flash memory organized in pages and rows, where each row
 * consists of 4 pages. While pages are writable one at a time, it is only
 * possible to delete a complete row. This implementation abstracts this
 * behavior by only writing complete rows at a time, so the FLASHPAGE_SIZE we
 * use in RIOT is actually the row size as specified in the datasheet.
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 *
 * @}
 */

#include <assert.h>
#include <string.h>

#include "cpu.h"
#include "macros/utils.h"
#include "periph/flashpage.h"
#include "unaligned.h"

#define ENABLE_DEBUG 0
#include "debug.h"

/* Write Quad Word is the only allowed operation on AUX pages */
#if defined(NVMCTRL_CTRLB_CMD_WQW)
#define AUX_CHUNK_SIZE  (4 * sizeof(uint32_t))
#elif defined(AUX_PAGE_SIZE)
#define AUX_CHUNK_SIZE AUX_PAGE_SIZE
#else
#define AUX_CHUNK_SIZE FLASH_USER_PAGE_SIZE
#endif

/**
 * @brief   NVMCTRL selection macros
 */
#ifdef CPU_FAM_SAML11
#define _NVMCTRL NVMCTRL_SEC
#else
#define _NVMCTRL NVMCTRL
#endif

/**
 * @brief   The user must ensure that the driver is configured with a proper number of wait states
 *          when the CPU is running at high frequencies. (Don't know how to compute this)
 */
#define FLASHPAGE_READ_WAIT_STATES  3

static inline void wait_nvm_is_ready(void)
{
#ifdef NVMCTRL_STATUS_READY
    while (!_NVMCTRL->STATUS.bit.READY) {}
#else
    while (!_NVMCTRL->INTFLAG.bit.READY) {}
#endif
}

static void _unlock(void)
{
    /* remove peripheral access lock for the NVMCTRL peripheral */
#ifdef REG_PAC_WRCTRL
    PAC->WRCTRL.reg = (PAC_WRCTRL_KEY_CLR | ID_NVMCTRL);
#else
    PAC1->WPCLR.reg = PAC1_WPROT_DEFAULT_VAL;
#endif
}

static void _lock(void)
{
    wait_nvm_is_ready();

    /* put peripheral access lock for the NVMCTRL peripheral */
#ifdef REG_PAC_WRCTRL
    PAC->WRCTRL.reg = (PAC_WRCTRL_KEY_SET | ID_NVMCTRL);
#else
    PAC1->WPSET.reg = PAC1_WPROT_DEFAULT_VAL;
#endif

    /* cached flash contents may have changed - invalidate cache */
#ifdef CMCC
    CMCC->MAINT0.bit.INVALL = 1;
#endif
}

static void _cmd_clear_page_buffer(void)
{
    wait_nvm_is_ready();

#ifdef NVMCTRL_CTRLB_CMDEX_KEY
    _NVMCTRL->CTRLB.reg = (NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC);
#else
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC);
#endif
}

static void _cmd_erase_aux(void)
{
    wait_nvm_is_ready();

    /* send Erase Page/Auxiliary Row command */
#if defined(NVMCTRL_CTRLB_CMD_EP)
    _NVMCTRL->CTRLB.reg = (NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EP);
#elif defined(NVMCTRL_CTRLA_CMD_EAR)
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_EAR);
#else
    /* SAML1x uses same command for all areas */
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER);
#endif
}

static void _cmd_erase_row(void)
{
    wait_nvm_is_ready();

    /* send Row/Block erase command */
#ifdef NVMCTRL_CTRLB_CMDEX_KEY
    _NVMCTRL->CTRLB.reg = (NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB);
#else
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER);
#endif
}

static void _cmd_write_aux(void)
{
    wait_nvm_is_ready();

    /* write auxiliary page */
#if defined(NVMCTRL_CTRLA_CMD_WAP)
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WAP);
#elif defined(NVMCTRL_CTRLB_CMD_WQW)
    _NVMCTRL->CTRLB.reg = (NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WQW);
#else
    /* SAML1x uses same command for all areas */
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP);
#endif
}

static void _cmd_write_page(void)
{
    wait_nvm_is_ready();

    /* write page */
#ifdef NVMCTRL_CTRLB_CMDEX_KEY
    _NVMCTRL->CTRLB.reg = (NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WP);
#else
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP);
#endif
}

/* We have to write whole words, but writing 0xFF is basically a no-op
 * so fill the unaligned bytes with 0xFF to get a whole extra word.
 * To support writes of data with less than 4 bytes, an offset needs
 * to be supplied.
 */
static uint32_t unaligned_pad_start(const void *_data, uint8_t len, uint8_t offset)
{
    assert((4 - offset) >= len);

    const uint8_t *data = _data;
    union {
        uint32_t u32;
        uint8_t  u8[4];
    } buffer = {.u32 = ~0};

    switch (len) {
        case 3:
            buffer.u8[offset + len - 3] = *data++;
            /* fall-through */
        case 2:
            buffer.u8[offset + len - 2] = *data++;
            /* fall-through */
        case 1:
            buffer.u8[offset + len - 1] = *data++;
    }

    return buffer.u32;
}

/* We have to write whole words, but writing 0xFF is basically a no-op
 * so fill the unaligned bytes with 0xFF to get a whole extra word.
 */
static uint32_t unaligned_pad_end(const void *_data, uint8_t len)
{
    const uint8_t *data = _data;
    union {
        uint32_t u32;
        uint8_t  u8[4];
    } buffer = {.u32 = ~0};

    switch (len) {
        case 3:
            buffer.u8[2] = data[2];
            /* fall-through */
        case 2:
            buffer.u8[1] = data[1];
            /* fall-through */
        case 1:
            buffer.u8[0] = data[0];
    }

    return buffer.u32;
}

static void _write_page(void* dst, const void *data, size_t len, void (*cmd_write)(void))
{
    /* set bytes in the first, unaligned word */
    uint8_t offset_unaligned_start = (uintptr_t)dst & 0x3;
    /* use MIN to support short data sizes below 3 bytes */
    uint8_t len_unaligned_start = MIN((4 - offset_unaligned_start) & 0x3, len);
    len -= len_unaligned_start;

    /* set bytes in the last, unaligned word */
    uint8_t len_unaligned_end = len & 0x3;
    len -= len_unaligned_end;

    /* word align destination address */
    uint32_t *dst32 = (void*)((uintptr_t)dst & ~0x3);

    _unlock();
    _cmd_clear_page_buffer();

    /* write the first, unaligned bytes */
    if (len_unaligned_start) {
        *dst32++ = unaligned_pad_start(data, len_unaligned_start,
                offset_unaligned_start);
        data = (void *)((uintptr_t)data + len_unaligned_start);
    }

    /* copy whole words */
    while (len) {
        /* due to unknown input data alignment and the conditional
         * shift applied above, data might not be aligned to a 4 byte
         * boundary at this point
         */
        *dst32++ = unaligned_get_u32(data);
        data = (void *)((uintptr_t)data + sizeof(uint32_t));
        len -= sizeof(uint32_t);
    }

    /* write the last, unaligned bytes */
    if (len_unaligned_end) {
        *dst32 = unaligned_pad_end(data, len_unaligned_end);
    }

    cmd_write();
    _lock();
}

static void _erase_page(void* page, void (*cmd_erase)(void))
{
    uintptr_t page_addr = (uintptr_t)page;

    /* erase given page (the ADDR register uses 16-bit addresses) */
    _unlock();

    /* ADDR drives the hardware (16-bit) address to the NVM when a command is executed using CMDEX.
     * 8-bit addresses must be shifted one bit to the right before writing to this register.
     */
#if defined(CPU_COMMON_SAMD21) || defined(CPU_COMMON_SAML21)
    page_addr >>= 1;
#endif

    /* set Row/Block start address */
    _NVMCTRL->ADDR.reg = page_addr;

    cmd_erase();
    _lock();
}

static void _write_row(uint8_t *dst, const void *_data, size_t len, size_t chunk_size,
                       void (*cmd_write)(void))
{
    const uint8_t *data = _data;

    size_t next_chunk = chunk_size - ((uintptr_t)dst & (chunk_size - 1));
    next_chunk = next_chunk ? next_chunk : chunk_size;

    while (len) {
        size_t chunk = MIN(len, next_chunk);
        next_chunk = chunk_size;

        _write_page(dst, data, chunk, cmd_write);
        data += chunk;
        dst  += chunk;
        len  -= chunk;
    }
}

#ifdef isr_nvmctrl
/**
 * @brief   NVMCTRL ISR
 */
void isr_nvmctrl(void)
{
    NVMCTRL_INTFLAG_Type intflag = _NVMCTRL->INTFLAG;
#if defined(NVMCTRL_INTFLAG_NSCHK)
    if (intflag.reg & NVMCTRL_INTFLAG_NSCHK) {
        DEBUG("NVMCTRL: Non-secure check\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_KEYE)
    if (intflag.reg & NVMCTRL_INTFLAG_KEYE) {
        DEBUG("NVMCTRL: Key error\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_NVME)
    if (intflag.reg & NVMCTRL_INTFLAG_NVME) {
        DEBUG("NVMCTRL: Non-volatile memory error\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_LOCKE)
    if (intflag.reg & NVMCTRL_INTFLAG_LOCKE) {
        DEBUG("NVMCTRL: Lock error\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_ECCSE) && defined(NVMCTRL_INTFLAG_ECCDE)
    if ((intflag.reg & NVMCTRL_INTFLAG_ECCSE) ||
        (intflag.reg & NVMCTRL_INTFLAG_ECCDE)) {
        if (intflag.reg & NVMCTRL_INTFLAG_ECCSE) {
            DEBUG("NVMCTRL: ECC single bit error\n");
        }
        if (intflag.reg & NVMCTRL_INTFLAG_ECCDE) {
            DEBUG("NVMCTRL: ECC double bit error\n");
        }
#if defined(NVMCTRL_ECCERR_OFFSET)
        NVMCTRL_ECCERR_Type eccerr = NVMCTRL->ECCERR;
#if defined(NVMCTRL_ECCERR_ADDR_Pos)
        DEBUG("NVMCTRL: ECC error in quad word at address: 0x%08lx\n",
              (eccerr.reg & NVMCTRL_ECCERR_ADDR_Msk) >> NVMCTRL_ECCERR_ADDR_Pos);
#endif
#if defined(NVMCTRL_ECCERR_TYPEL_Pos)
        DEBUG("NVMCTRL: ECC error in low byte: 0x%08lx\n",
              (eccerr.reg & NVMCTRL_ECCERR_TYPEL_Msk) >> NVMCTRL_ECCERR_TYPEL_Pos);
#endif
#if defined(NVMCTRL_ECCERR_TYPEH_Pos)
        DEBUG("NVMCTRL: ECC error in high byte: 0x%08lx\n",
              (eccerr.reg & NVMCTRL_ECCERR_TYPEH_Msk) >> NVMCTRL_ECCERR_TYPEH_Pos);
#endif
    }
#endif
#endif
#if defined(NVMCTRL_INTFLAG_PROGE)
    if (intflag.reg & NVMCTRL_INTFLAG_PROGE) {
        DEBUG("NVMCTRL: Programming error\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_ADDRE)
    if (intflag.reg & NVMCTRL_INTFLAG_ADDRE) {
        DEBUG("NVMCTRL: Address error\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_ERROR)
    if (intflag.reg & NVMCTRL_INTFLAG_ERROR) {
        DEBUG("NVMCTRL: Error\n");
        NVMCTRL_STATUS_Type status = _NVMCTRL->STATUS;
#if defined(NVMCTRL_STATUS_NVME)
        if (status.reg & NVMCTRL_STATUS_NVME) {
            DEBUG("NVMCTRL: Non-volatile memory error\n");
            _NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_NVME;
        }
#endif
#if defined(NVMCTRL_STATUS_LOCKE)
        if (status.reg & NVMCTRL_STATUS_LOCKE) {
            DEBUG("NVMCTRL: Lock error\n");
            _NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_LOCKE;
        }
#endif
#if defined(NVMCTRL_STATUS_PROGE)
        if (status.reg & NVMCTRL_STATUS_PROGE) {
            DEBUG("NVMCTRL: Programming error\n");
            _NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_PROGE;
        }
#endif
    }
#endif
#if defined(NVMCTRL_INTFLAG_DONE)
    if (intflag.reg & NVMCTRL_INTFLAG_DONE) {
        DEBUG("NVMCTRL: Operation done\n");
    }
#endif
#if defined(NVMCTRL_INTFLAG_READY)
    if (intflag.reg & NVMCTRL_INTFLAG_READY) {
        DEBUG("NVMCTRL: NVMCTRL ready\n");
    }
#endif
    /* reset interrupt flags */
    _NVMCTRL->INTFLAG.reg = NVMCTRL_INTFLAG_MASK;
    cortexm_isr_end();
}
#endif

void flashpage_init(void)
{
#ifdef NVMCTRL_IRQn
    NVIC_EnableIRQ(NVMCTRL_IRQn);
#endif
    _unlock();
    /* set power reduction mode to best power saving mode (wakeup on first access) */
#ifdef NVMCTRL_CTRLA_PRM
    _NVMCTRL->CTRLA.reg &= ~NVMCTRL_CTRLA_PRM_Msk;
    _NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_PRM_SEMIAUTO;
#elif defined(NVMCTRL_CTRLB_SLEEPPRM)
    _NVMCTRL->CTRLB.reg &= ~NVMCTRL_CTRLB_SLEEPPRM_Msk;
    _NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS;
#endif
    /* set automatic wait states (depends on AHB bus frequency) */
#ifdef NVMCTRL_CTRLA_RWS
    _NVMCTRL->CTRLA.reg &= ~(NVMCTRL_CTRLA_RWS_Msk);
    _NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_AUTOWS;
#elif defined(NVMCTRL_CTRLB_RWS)
    _NVMCTRL->CTRLB.reg &= ~(NVMCTRL_CTRLB_RWS_Msk);
    _NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_RWS(FLASHPAGE_READ_WAIT_STATES);
#endif
    /* set write mode to manual */
#ifdef NVMCTRL_CTRLA_WMODE
    _NVMCTRL->CTRLA.reg &= ~(NVMCTRL_CTRLA_WMODE_Msk);
    _NVMCTRL->CTRLA.reg |= NVMCTRL_CTRLA_WMODE_MAN;
#elif defined(NVMCTRL_CTRLB_MANW)
    _NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_MANW;
#endif
    /* disable cache lines */
#ifdef NVMCTRL_CTRLA_CACHEDIS0
    _NVMCTRL->CTRLA.reg |= (NVMCTRL_CTRLA_CACHEDIS0 | NVMCTRL_CTRLA_CACHEDIS1);
#elif defined(NVMCTRL_CTRLB_CACHEDIS)
    _NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_CACHEDIS;
#endif
    /* ECC errors from the debugger when CPU is halted in debug mode shall not be logged */
#ifdef NVMCTRL_DBGCTRL_ECCDIS
    _NVMCTRL->DBGCTRL.reg |= NVMCTRL_DBGCTRL_ECCDIS;
    _NVMCTRL->DBGCTRL.reg &= ~(NVMCTRL_DBGCTRL_ECCELOG);
#endif
    /* enable all interrupts */
    _NVMCTRL->INTFLAG.reg = NVMCTRL_INTFLAG_MASK;
    _lock();
}

void flashpage_erase(unsigned page)
{
    assert((unsigned)page < FLASHPAGE_NUMOF);
    _erase_page(flashpage_addr(page), _cmd_erase_row);
}

void flashpage_write(void *target_addr, const void *data, size_t len)
{
    /* ensure the length doesn't exceed the actual flash size */
    assert(((unsigned)target_addr + len) <=
           (CPU_FLASH_BASE + (FLASHPAGE_SIZE * FLASHPAGE_NUMOF)));

    _write_row(target_addr, data, len, NVMCTRL_PAGE_SIZE, _cmd_write_page);
}

void sam0_flashpage_aux_write(uint32_t offset, const void *data, size_t len)
{
    uintptr_t dst = NVMCTRL_USER + sizeof(nvm_user_page_t) + offset;

#ifdef FLASH_USER_PAGE_SIZE
    assert(dst + len <= NVMCTRL_USER + FLASH_USER_PAGE_SIZE);
#else
    assert(dst + len <= NVMCTRL_USER + AUX_PAGE_SIZE * AUX_NB_OF_PAGES);
#endif

    _write_row((void*)dst, data, len, AUX_CHUNK_SIZE, _cmd_write_aux);
}

static void _debug_print_user_cfg(const nvm_user_page_t *cfg)
{
    (void)cfg;
    DEBUG_PUTS("NVM User Row:");
    /* config is a bitfield */
#if defined(CPU_COMMON_SAMD5X)
    DEBUG("BOD33 disable:               0x%x\n", cfg->bod33_disable);
    DEBUG("BOD33 level:                 0x%x\n", cfg->bod33_level);
    DEBUG("BOD33 action:                0x%x\n", cfg->bod33_action);
    DEBUG("BOD33 hysteresis:            0x%x\n", cfg->bod33_hysteresis);
    DEBUG("BID12 factory calibration:   0x%x\n", cfg->bod12_calibration);
    DEBUG("NVM bootloader size:         0x%x (%u K)\n", cfg->nvm_boot_size,
                                                        (15 - cfg->nvm_boot_size) * 8);
    DEBUG("SBLK:                        0x%x\n", cfg->smart_eeprom_blocks);
    DEBUG("PSZ:                         0x%x\n", cfg->smart_eeprom_page_size);
    DEBUG("RAM ECCDIS:                  0x%x\n", cfg->ram_eccdis);
    DEBUG("WDT enable:                  0x%x\n", cfg->wdt_enable);
    DEBUG("WDT always on:               0x%x\n", cfg->wdt_always_on);
    DEBUG("WDT period:                  0x%x\n", cfg->wdt_period);
    DEBUG("WDT window:                  0x%x\n", cfg->wdt_window);
    DEBUG("WDT early warning offset:    0x%x\n", cfg->wdt_ewoffset);
    DEBUG("WDT window enable:           0x%x\n", cfg->wdt_window_enable);
    DEBUG("NVM locks:                   0x%08lx\n", cfg->nvm_locks);
#elif defined(CPU_COMMON_SAMD21) || defined(CPU_COMMON_SAML21)
    DEBUG("NVM bootloader size:         0x%x (%u 256B)\n", cfg->bootloader_size,
                                                           1u << (7 - cfg->bootloader_size));
    DEBUG("EEPROM size:                 0x%x\n", cfg->eeprom_size);
    DEBUG("BOD33 level:                 0x%x\n", cfg->bod33_level);
#if defined(CPU_COMMON_SAML21)
    DEBUG("BOD33 disable:               0x%x\n", cfg->bod33_disable);
#elif defined(CPU_COMMON_SAMD21)
    DEBUG("BOD33 enable:                0x%x\n", cfg->bod33_enable);
#endif
    DEBUG("BOD33 action:                0x%x\n", cfg->bod33_action);
    DEBUG("BOD12 calibration:           0x%x\n", cfg->bod12_calibration);
    DEBUG("WDT enable:                  0x%x\n", cfg->wdt_enable);
    DEBUG("WDT always on:               0x%x\n", cfg->wdt_always_on);
    DEBUG("WDT period:                  0x%x\n", cfg->wdt_period);
    DEBUG("WDT window:                  0x%x\n", cfg->wdt_window);
    DEBUG("WDT early warning offset:    0x%x\n", cfg->wdt_ewoffset);
    DEBUG("WDT window enable:           0x%x\n", cfg->wdt_window_enable);
    DEBUG("BOD33 hysteresis:            0x%x\n", cfg->bod33_hysteresis);
    DEBUG("NVM locks:                   0x%04x\n", cfg->nvm_locks);
#elif defined(CPU_COMMON_SAML1X)
    DEBUG("SULCK:                       0x%x\n", cfg->secure_region_unlock);
    DEBUG("NSULCK:                      0x%x\n", cfg->non_secure_region_unlock);
    DEBUG("BOD33 level:                 0x%x\n", cfg->bod33_level);
    DEBUG("BOD33 disable:               0x%x\n", cfg->bod33_disable);
    DEBUG("BOD33 action:                0x%x\n", cfg->bod33_action);
    DEBUG("BOD12 calibration:           0x%x\n", cfg->bod12_calibration);
    DEBUG("WDT run standby:             0x%x\n", cfg->wdt_run_standby);
    DEBUG("WDT enable:                  0x%x\n", cfg->wdt_enable);
    DEBUG("WDT always on:               0x%x\n", cfg->wdt_always_on);
    DEBUG("WDT period:                  0x%x\n", cfg->wdt_period);
    DEBUG("WDT window:                  0x%x\n", cfg->wdt_window);
    DEBUG("WDT early warning offset:    0x%x\n", cfg->wdt_ewoffset);
    DEBUG("WDT window enable:           0x%x\n", cfg->wdt_window_enable);
    DEBUG("BOD33 hysteresis:            0x%x\n", cfg->bod33_hysteresis);
    DEBUG("RXN:                         0x%x\n", cfg->ram_execute_never);
    DEBUG("DXN:                         0x%x\n", cfg->data_execute_never);
    DEBUG("AS:                          0x%x\n", cfg->secure_flash_as_size);
    DEBUG("ANSC:                        0x%x\n", cfg->nsc_size);
    DEBUG("DS:                          0x%x\n", cfg->secure_flash_data_size);
    DEBUG("RS:                          0x%x\n", cfg->secure_ram_size);
    DEBUG("URWEN:                       0x%x\n", cfg->user_row_write_enable);
    DEBUG("NOSECA:                      0x%lx\n", cfg->nonsec_a);
    DEBUG("NOSECB:                      0x%lx\n", cfg->nonsec_b);
    DEBUG("NOSECC:                      0x%lx\n", cfg->nonsec_c);
    DEBUG("USERCRC:                     0x%lx\n", cfg->user_crc);
#endif
}

void sam0_flashpage_aux_reset(const nvm_user_page_t *cfg)
{
    nvm_user_page_t old_cfg;

    if (cfg == NULL) {
        cfg = &old_cfg;
        memcpy(&old_cfg, (void*)NVMCTRL_USER, sizeof(*cfg));
    }

    _debug_print_user_cfg(cfg);
    _erase_page((void*)NVMCTRL_USER, _cmd_erase_aux);
    _write_row((void*)NVMCTRL_USER, cfg, sizeof(*cfg), AUX_CHUNK_SIZE, _cmd_write_aux);
}

void sam0_flashpage_aux_restore(void)
{
    nvm_user_page_t cfg;
    sam0_aux_config_init_default(&cfg);
    sam0_flashpage_aux_reset(&cfg);
}

#ifdef FLASHPAGE_RWWEE_NUMOF

static void _cmd_erase_row_rwwee(void)
{
    wait_nvm_is_ready();

    /* send erase row command */
#ifdef NVMCTRL_CTRLA_CMD_RWWEEER
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_RWWEEER);
#else
    /* SAML1X use the same Erase command for both flash memories */
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER);
#endif
}

static void _cmd_write_page_rwwee(void)
{
    wait_nvm_is_ready();

    /* write page */
#ifdef NVMCTRL_CTRLA_CMD_RWWEEWP
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_RWWEEWP);
#else
    /* SAML1X use the same Write Page command for both flash memories */
    _NVMCTRL->CTRLA.reg = (NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP);
#endif
}

void flashpage_rwwee_write(void *target_addr, const void *data, size_t len)
{
    assert(((unsigned)target_addr + len) <=
           (CPU_FLASH_RWWEE_BASE + (FLASHPAGE_SIZE * FLASHPAGE_RWWEE_NUMOF)));

    _write_row(target_addr, data, len, NVMCTRL_PAGE_SIZE, _cmd_write_page_rwwee);
}

void flashpage_rwwee_write_page(unsigned page, const void *data)
{
    assert((unsigned)page < FLASHPAGE_RWWEE_NUMOF);

    _erase_page(flashpage_rwwee_addr(page), _cmd_erase_row_rwwee);

    if (data == NULL) {
        return;
    }

    /* One RIOT page is FLASHPAGE_PAGES_PER_ROW SAM0 flash pages (a row) as
     * defined in the file cpu/sam0_common/include/cpu_conf.h, therefore we
     * have to split the write into FLASHPAGE_PAGES_PER_ROW raw calls
     * underneath, each writing a physical page in chunks of 4 bytes (see
     * flashpage_write_raw)
     * The erasing is done once as a full row is always erased.
     */
    _write_row(flashpage_rwwee_addr(page), data, FLASHPAGE_SIZE, NVMCTRL_PAGE_SIZE,
               _cmd_write_page_rwwee);
}
#endif /* FLASHPAGE_RWWEE_NUMOF */
