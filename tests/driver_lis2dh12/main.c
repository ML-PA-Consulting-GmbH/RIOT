/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the LIS2DH12 accelerometer driver
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "fmt.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "board.h"
#include "lis2dh12.h"
#include "lis2dh12_params.h"


#define DELAY       (100UL * US_PER_MS)

/* allocate some memory to hold one formated string for each sensor output, so
 * one string for the X, Y, and Z reading, respectively */
static char str_out[3][8];

/* allocate device descriptor */
static lis2dh12_t dev;

/* interrupt pins*/
//TODO: change the interruptpins to real connected
char* irq_1 = "PA12"; // "NC" to disable interrupt 1 (not connected)
char* irq_2 = "PA13"; // "NC" to disable interrupt 2 (not connected)

/* interrupt callback function. */
static void int_cb(void* pin){
    printf("interrupt received from %s\n", (char*)pin);

    int_src_reg_t buffer = {0};

    if(!strcmp(irq_1,pin)){
        lis2dh12_read_int_src(&dev,&buffer, 1);
    }
    else if(!strcmp(irq_2,pin)){
        lis2dh12_read_int_src(&dev,&buffer, 2);
    }

    printf("content SRC_Reg:\n\t XL 0x%02x\n",buffer.LIS2DH12_INT_SRC_XL);
    printf("\t XH 0x%02x\n",buffer.LIS2DH12_INT_SRC_XH);
    printf("\t YL 0x%02x\n",buffer.LIS2DH12_INT_SRC_YL);
    printf("\t YH 0x%02x\n",buffer.LIS2DH12_INT_SRC_YH);
    printf("\t ZL 0x%02x\n",buffer.LIS2DH12_INT_SRC_ZL);
    printf("\t ZH 0x%02x\n",buffer.LIS2DH12_INT_SRC_ZH);
    printf("\t IA 0x%02x\n\n",buffer.LIS2DH12_INT_SRC_IA);
}

int main(void)
{
    puts("LIS2DH12 accelerometer driver test application\n");

    puts("Initializing LIS2DH12 sensor... ");
    if (lis2dh12_init(&dev, &lis2dh12_params[0]) == LIS2DH12_OK) {
        puts("[OK]");
    }
    else {
        puts("[Failed]\n");
        return 1;
    }

    /* enable interrupt Pins */
    if (strcmp("NC",irq_1))
        if (gpio_init_int(GPIO_PIN(PA,12),GPIO_IN, GPIO_RISING,int_cb,irq_1) == -1)
            puts("init_int failed!\n");

    if (strcmp("NC",irq_2))
        if (gpio_init_int(GPIO_PIN(PA,13),GPIO_IN, GPIO_RISING,int_cb,irq_2) == -1)
            puts("init_int failed!\n");

    /* create and set the interrupt params */
    int_params_t params = {0};
    params.int_type = 0b01000000;
    params.int_config = 1;
    params.int_threshold = 0b00011111;
    params.int_duration = 1;
    lis2dh12_set_int(&dev,params,1);

    xtimer_ticks32_t last_wakeup = xtimer_now();
    while (1) {
        /* read sensor data */
        int16_t data[3];
        if (lis2dh12_read(&dev, data) != LIS2DH12_OK) {
            puts("error: unable to retrieve data from sensor, quitting now");
            return 1;
        }

        /* format data */
        for (int i = 0; i < 3; i++) {
            size_t len = fmt_s16_dfp(str_out[i], data[i], -3);
            str_out[i][len] = '\0';
        }

        /* print data to STDIO */
        printf("X: %8s Y: %8s Z: %8s\n", str_out[0], str_out[1], str_out[2]);

        xtimer_periodic_wakeup(&last_wakeup, DELAY);
    }

    return 0;
}
