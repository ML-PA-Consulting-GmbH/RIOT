/*
 * Copyright (C) 2020 ML!PA Consulting GmbH
 *
 */

#include "stdio.h"
#include "stdlib.h"
#include "periph/pm.h"
#include "periph/rtc.h"
#include "periph/wdt.h"
#include "periph/adc.h"
#include "xtimer.h"

static void _rtc_cb(void* args) {
    (void) args;
    puts("rtc wakeup");
}

static uint32_t num_iter BACKUP_RAM_DATA;

int main(void)
{
    rtc_clear_alarm();
    printf(">>> STARTUP, RCAUSE = 0x%02x, ITER = %"PRIu32".\n", RSTC->RCAUSE.reg, num_iter);
    if (RSTC->RCAUSE.reg & RSTC_RCAUSE_WDT) {
        goto exit;
    }

    puts("Starting watchdog timer.");
    wdt_init();
    wdt_setup_reboot(0, PARAM_WATCHDOG_WINDOW_MAX_MS);
    wdt_start();

    puts("Initializing ADC.");
    adc_init(ADC_LINE(PARAM_TEST_ADC_LINE));

    do {
        puts("Kicking watchdog timer.");
        wdt_kick();

        puts("Sampling ADC.");
        num_iter++;
        int result = adc_sample(ADC_LINE(PARAM_TEST_ADC_LINE), ADC_RES_12BIT);
        printf("ADC result = %d.\n", result);

        if (num_iter >= PARAM_TEST_ITERATIONS) {
            wdt_stop();
            goto exit;
        }

        struct tm wakeup_time;
        rtc_get_time(&wakeup_time);;
        wakeup_time.tm_sec += PARAM_NSECS_HIBERNATE;
        rtc_tm_normalize(&wakeup_time);

        printf("Setting rtc to wake up in %i seconds.\n", PARAM_NSECS_HIBERNATE);
        rtc_set_alarm(&wakeup_time, _rtc_cb, NULL);

        printf("Setting power mode %d.\n", PARAM_SLEEP_LEVEL);
        fflush(stdout);
        wdt_stop();
        pm_set(PARAM_SLEEP_LEVEL);
        wdt_start();
        rtc_clear_alarm();
        /* offset to make RTC happy, it might
         * fail otherwise for non-hibernate sleep
         */
        xtimer_msleep(PARAM_POST_STANDBY_HOLDOFF_MS);
    } while (PARAM_SLEEP_LEVEL);

    /* this code should never be reached */
    puts("woke up again - THIS SHOULD NOT HAPPEN - rebooting...");
    pm_reboot();

 exit:
    puts(num_iter >= PARAM_TEST_ITERATIONS ? "SUCCESS" : "FAILURE");
    while (true) {}

    return 0;
}
