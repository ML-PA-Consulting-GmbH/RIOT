/*
 * Copyright (C) 2017 Freie Universität Berlin
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
 * @brief       Example for demonstrating SAUL and the SAUL registry
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "periph/gpio.h"
#include "lis2dh12.h"
#include "lis2dh12_params.h"

static lis2dh12_t dev;

static void int_cb(void* pin){
	puts("interrupt received from ");
	puts(pin);
	puts(" \n");

    uint8_t buffer = 0;

    if(!strcmp("PA12",pin)){
        lis2dh12_read_interrupt(&dev,&buffer, 1);
    }
    else{
        lis2dh12_read_interrupt(&dev,&buffer, 2);
    }


    puts("calc value\n");
   // char str[6] = {0};
    puts("sprint\n");
    //sprintf(str,"%02x",buffer);

    puts("sprint\n");
    printf("%d",buffer);
    puts("\n");
}

int main(void)
{
    puts("Welcome to RIOT!\n");
    puts("Type `help` for help, type `saul` to see all SAUL devices\n");

    // Interrupt Pins
    char* pin = "PA12";
    if(gpio_init_int(GPIO_PIN(PA,12),GPIO_IN, GPIO_RISING,int_cb,pin) == -1)
    	puts("init_int failed!\n");

    pin = "PA13";
    if(gpio_init_int(GPIO_PIN(PA,13),GPIO_IN, GPIO_RISING,int_cb,pin) == -1)
    	puts("init_int failed!\n");	


    lis2dh12_init(&dev,&lis2dh12_params[0]);


    //int_params_t params = {0};
    //params.cfg = 1;
    //lis2dh12_set_interrupt(&dev,params,1);


    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
