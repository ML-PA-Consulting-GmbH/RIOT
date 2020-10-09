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

void init_qspi(void){
    /* set Master Clock */
    MCLK->AHBMASK.reg  |= MCLK_AHBMASK_QSPI;
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_QSPI;

    /* init GPIOs */
    /* QSPI CS */
    gpio_init(GPIO_PIN(PB, 11),GPIO_IN);
    gpio_init_mux(GPIO_PIN(PB, 11), GPIO_MUX_H);
    /* QSPI DATA_0 */
    gpio_init(GPIO_PIN(PA, 8),GPIO_OUT);
    gpio_clear(GPIO_PIN(PA, 8));
    gpio_init_mux(GPIO_PIN(PA, 8), GPIO_MUX_H);
    /* QSPI DATA_1 */
    gpio_init(GPIO_PIN(PA, 9),GPIO_OUT);
    gpio_clear(GPIO_PIN(PA, 9));
    gpio_init_mux(GPIO_PIN(PA, 9), GPIO_MUX_H);
    /* QSPI DATA_2 */
    gpio_init(GPIO_PIN(PA, 10),GPIO_OUT);
    gpio_clear(GPIO_PIN(PA, 10));
    gpio_init_mux(GPIO_PIN(PA, 10), GPIO_MUX_H);
    /* QSPI DATA_3 */
    gpio_init(GPIO_PIN(PA, 11),GPIO_OUT);
    gpio_clear(GPIO_PIN(PA, 11));
    gpio_init_mux(GPIO_PIN(PA, 11), GPIO_MUX_H);
    /* QSPI CLK */
    gpio_init(GPIO_PIN(PB, 10),GPIO_IN);
    gpio_init_mux(GPIO_PIN(PB, 10), GPIO_MUX_H);

    /* Settings */
    QSPI->CTRLA.reg = 1; //SWRST

    QSPI->CTRLB.reg = 0x06000011;
    /*QSPI->CTRLB.bit.MODE = 1; //MemoryMode
    QSPI->CTRLB.bit.CSMODE = 1; //LASTXFER
    QSPI->CTRLB.bit.DATALEN = 0; //Length 8Bit
    QSPI->CTRLB.bit.DLYBCT = 0; //ignored in MemoryMode
    QSPI->CTRLB.bit.DLYCS = 6; //CS delay*/

    QSPI->BAUD.reg = 0x00241300;
    /*QSPI->BAUD.bit.CPOL = 0;
    QSPI->BAUD.bit.CPHA = 0;
    QSPI->BAUD.bit.BAUD = 19; //(120MHz/6MHz)-1
    QSPI->BAUD.bit.DLYBS = 36; //120MHZ*0,3us (300ns delay)*/
}

void clear_ints(void){
    puts("clear ints");
    //clear all interrupts
    QSPI->INTFLAG.reg = QSPI->INTFLAG.reg;
    while((QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");
}

void qspi_erase(void){

    puts("erase Chip");
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);

    // erase chip
    QSPI->INSTRADDR.reg = 22;
    QSPI->INSTRCTRL.reg  = 6;
    QSPI->INSTRFRAME.reg = 22;

    QSPI->INSTRADDR.reg = 144;
    QSPI->INSTRCTRL.reg  = 5;
    QSPI->INSTRFRAME.reg = 150;

    /* read for sync */
    uint32_t tmp = QSPI->INSTRFRAME.reg;
    fprintf(stdout,"tmp: %lx\n", tmp);

    QSPI->CTRLB.bit.MODE = 1;
    QSPI->CTRLA.bit.ENABLE = 1;
    while(!QSPI->STATUS.bit.ENABLE);


    QSPI->CTRLA.bit.LASTXFER = 1;

    while(!(QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"instrend: %x\n", (QSPI->INTFLAG.bit.INSTREND));
    fprintf(stdout,"status: %lx\n", QSPI->STATUS.reg);
    puts("done\n");
}

void qspi_write(void){
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

}

void qspi_read(void){
    puts("read");

    QSPI->CTRLA.reg = 2;
    QSPI->CTRLB.reg = 1;
    while(!QSPI->STATUS.bit.ENABLE);

    QSPI->INSTRADDR.reg  = 144;
    QSPI->INSTRCTRL.reg  = 97;
    QSPI->INSTRFRAME.reg = 8336;

    //dummy read
    uint32_t tmp = QSPI->INSTRFRAME.reg;
    (void) tmp;

    QSPI->CTRLA.reg = 0x01000002;

    while(!(QSPI->INTFLAG.bit.INSTREND));

    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);
    puts("done\n");

}


int main(void)
{
    /* Init QSPI */
    init_qspi();

    /* QSPI enable */
    QSPI->CTRLA.reg = 2;

    QSPI->INSTRADDR.reg = 144;
    QSPI->INSTRCTRL.reg = 101;
    QSPI->INSTRFRAME.reg = 144;

    /* treansfer */
    //dummy read, to sync bus
    uint32_t tmp = QSPI->INSTRFRAME.reg;
    (void) tmp;

    /* QSPI end transfer */
    QSPI->CTRLA.reg = 0x01000002;

    /* run command */
    do {
        QSPI->INSTRADDR.reg = 16;
        QSPI->INSTRCTRL.reg = 6;
        QSPI->INSTRFRAME.reg = 16;
        tmp = QSPI->INSTRFRAME.reg;
        (void) tmp;

        QSPI->INSTRADDR.reg = 144;
        QSPI->INSTRCTRL.reg = 5;
        QSPI->INSTRFRAME.reg = 144;
        tmp = QSPI->INSTRFRAME.reg;
        (void) tmp;
    } while(!QSPI->STATUS.bit.ENABLE);


    /* QSPI end transfer */
    QSPI->CTRLA.reg = 0x01000002;

    QSPI->INSTRADDR.reg = 144;
    QSPI->INSTRCTRL.reg = 97;
    QSPI->INSTRFRAME.reg = 8336;

    tmp = QSPI->INSTRFRAME.reg;
    (void) tmp;

    while(!(QSPI->INTFLAG.bit.INSTREND));

    fprintf(stdout,"DATA: 0x%lx\n", QSPI->RXDATA.reg);

    puts("init done\n");

    clear_ints();

    qspi_read();

    clear_ints();

    return 0;
}
