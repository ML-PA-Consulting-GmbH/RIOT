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
    /* init GPIOs for QSPI */
    gpio_init(GPIO_PIN(PB, 10),GPIO_OUT);
    gpio_init(GPIO_PIN(PB, 11),GPIO_OUT);
    gpio_init_mux(GPIO_PIN(PA,  8), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PA,  9), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PA, 10), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PA, 11), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PB, 10), GPIO_MUX_H);
    gpio_init_mux(GPIO_PIN(PB, 11), GPIO_MUX_H);

    /* set Master Clock */
    MCLK->AHBMASK.reg  |= MCLK_AHBMASK_QSPI;
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_QSPI;

    /*0x9F 
    Baud 375000
    polarity zero
    phase changed on leading, captured on falling*/


    /* activate QSPI module */
    QSPI->CTRLA.bit.ENABLE = 1;

    QSPI->CTRLB.bit.DLYCS = 0;
    QSPI->CTRLB.bit.DLYBCT = 0;
    QSPI->CTRLB.bit.DATALEN = 0; //8Bit
    QSPI->CTRLB.bit.CSMODE = 1;
    QSPI->CTRLB.bit.SMEMREG = 1;
    QSPI->CTRLB.bit.WDRBT = 0;
    QSPI->CTRLB.bit.LOOPEN = 0;
    QSPI->CTRLB.bit.MODE = 1;

    QSPI->BAUD.bit.BAUD = 19; //MCLK 120MHZ -> 6MHZ
    QSPI->BAUD.bit.DLYBS = 9; //DLYBS/MCLK -> 300ns
    QSPI->BAUD.bit.BAUD = 9;
    QSPI->BAUD.bit.CPHA = 1;
    QSPI->BAUD.bit.CPOL = 0;

    while(!QSPI->STATUS.bit.ENABLE);

    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("init done\n");
    fprintf(stdout,"baud: 0x%lx\n", QSPI->BAUD.reg);
    


    puts("erase Chip");
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);

    // erase chip
    QSPI->INSTRCTRL.reg  = 0x000000C7;
    QSPI->INSTRFRAME.reg = 0x00000010;

    QSPI->CTRLB.bit.MODE = 1;
    QSPI->CTRLA.bit.ENABLE = 1;
    while(!QSPI->STATUS.bit.ENABLE);

    while(!(QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");

    puts("clear ints");
    //clear all interrupts
    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");

    puts("write");
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    //write to chip
    //QSPI->INSTRADDR.reg  = 0x04000000;

    QSPI->CTRLB.bit.MODE = 1;
    QSPI->CTRLA.bit.ENABLE = 1;
    while(!QSPI->STATUS.bit.ENABLE);

    
    QSPI->INSTRCTRL.reg  = 0x00000002;
    QSPI->INSTRFRAME.reg = 0x000030B3;

    //dummy read
    fprintf(stdout,"Frame: %lx\n", QSPI->INSTRFRAME.reg);

    QSPI->TXDATA.reg = 0xFF;

    QSPI->CTRLA.bit.LASTXFER = 1;

    //fprintf(stdout,"DATA: 0x%lx\n", QSPI->TXDATA.reg);

    while(!(QSPI->INTFLAG.bit.INSTREND));
    //while(!(QSPI->INTFLAG.bit.TXC));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");

    puts("clear ints");
    //clear all interrupts
    fprintf(stdout,"Ints: 0x%lx\n", QSPI->INTFLAG.reg);

    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);

    fprintf(stdout,"Ints: 0x%lx\n", QSPI->INTFLAG.reg);
    puts("done\n");

    puts("read");

    QSPI->CTRLB.bit.MODE = 1;
    QSPI->CTRLA.bit.ENABLE = 1;
    while(!QSPI->STATUS.bit.ENABLE);

    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    //QSPI->INSTRADDR.reg  = 0x04000000;
    QSPI->INSTRCTRL.reg  = 0x0000000B;
    QSPI->INSTRFRAME.reg = 0x000220B6;

    //dummy read
    fprintf(stdout,"Frame: %lx\n", QSPI->INSTRFRAME.reg);

    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);

    QSPI->CTRLA.bit.LASTXFER = 1;

    while(!(QSPI->INTFLAG.bit.INSTREND))    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);

    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");

    puts("clear ints");
    //clear all interrupts
    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");

    return 0;
}
