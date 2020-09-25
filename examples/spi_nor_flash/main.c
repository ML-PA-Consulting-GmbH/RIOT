/*
 * Copyright (C) 2018 OTA keys S.A.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       File system usage example application
 *
 * @author      Vincent Dupont <vincent@otakeys.com>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "board.h" /* MTD_0 is defined in board.h */
#include "periph/gpio.h"
#include "periph/spi.h"

int main(void)
{

   
    /* set WRITEPROTECT and HOLD to high*/
    /*gpio_init(GPIO_PIN(PA, 10),GPIO_OUT);
    gpio_set(GPIO_PIN(PA, 10));
    gpio_init(GPIO_PIN(PA, 11),GPIO_OUT);
    gpio_set(GPIO_PIN(PA, 11));
    puts("done!\n");
#if defined(MTD_0) && (defined(MODULE_SPIFFS) || defined(MODULE_LITTLEFS))

    fs_desc.dev = MTD_0;
#elif defined(MTD_0) && defined(MODULE_FATFS_VFS)
    fatfs_mtd_devs[fs_desc.vol_idx] = MTD_0;
#endif
    int res = vfs_mount(&const_mount);
    if (res < 0) {
        puts("Error while mounting constfs");
    }
    else {
        puts("constfs mounted successfully");
    }

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);*/


    puts("Hello\n");

    MCLK->AHBMASK.reg  |= MCLK_AHBMASK_QSPI;
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_QSPI;

    gpio_init(GPIO_PIN(PB, 10),GPIO_OUT);
    gpio_init(GPIO_PIN(PB, 11),GPIO_OUT);
    gpio_init_mux(GPIO_PIN(PA,  8), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PA,  9), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PA, 10), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PA, 11), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PB, 10), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PB, 11), GPIO_MUX_H);


    QSPI->CTRLB.bit.MODE = 1;
    QSPI->CTRLA.bit.ENABLE = 1;
    
    while(!QSPI->STATUS.bit.ENABLE);

    fprintf(stdout,"INSTREND: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("init done");

    // erase chip
    QSPI->INSTRCTRL.reg  = 0x000000C7;
    QSPI->INSTRFRAME.reg = 0x00000010;
    
    fprintf(stdout,"INSTREND: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    while(!(QSPI->INTFLAG.bit.INSTREND));
    puts("done");

    //clear all interrupts
    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));

    //write to chip
    fprintf(stdout,"INSTREND: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    QSPI->INSTRADDR.reg  = 0x04000000;
    QSPI->TXDATA.reg =     0xFFFFFFFF;

    QSPI->INSTRCTRL.reg  = 0x00000002;
    QSPI->INSTRFRAME.reg = 0x000030B3;
    QSPI->CTRLA.bit.LASTXFER = 1;

    fprintf(stdout,"INSTREND: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    while(!(QSPI->INTFLAG.bit.INSTREND));

    puts("write done");
    //clear all interrupts
    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));


    QSPI->INSTRADDR.reg  = 0x04000000;
    QSPI->INSTRCTRL.reg  = 0x0000006B;
    QSPI->INSTRFRAME.reg = 0x000810B2;
    QSPI->CTRLA.bit.LASTXFER = 1;

    while(!(QSPI->INTFLAG.bit.INSTREND));
    puts("read done");
    fprintf(stdout,"data: %lx\n", QSPI->RXDATA.reg);
    //clear all interrupts
    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));

    return 0;
}
