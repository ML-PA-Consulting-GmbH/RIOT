/*
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
 * @brief       Test application for the candev abstraction
 *
 *
 * @}
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can/device.h"

#include "can_params.h"
#include "periph/can.h"
#include "ztimer.h"
#include "shell.h"

static can_t can_samd5x;
candev_t *candev = NULL;

#ifndef SAMD5X_CAN_TEST_MODE
#define SAMD5X_CAN_TEST_MODE    0
#endif

#define ENABLE_DEBUG 0
#include <debug.h>

struct can_frame frame_1 = {
        .can_dlc = 5,
        .can_id = 0x8C000001,
        .data = {
            4,
            1,
            2,
            3,
            0xF
        }
    };

static void _can_event_callback(candev_t *dev, candev_event_t event, void *arg)
{
    (void)dev;
    struct can_frame *frame = arg;

    switch (event) {
    case CANDEV_EVENT_ISR:
        DEBUG("_can_event: CANDEV_EVENT_ISR\n");
        dev->driver->isr(candev);
        break;
    case CANDEV_EVENT_WAKE_UP:
        DEBUG("_can_event: CANDEV_EVENT_WAKE_UP\n");
        break;
    case CANDEV_EVENT_TX_CONFIRMATION:
        DEBUG("_can_event: CANDEV_EVENT_TX_CONFIRMATION\n");
        frame = (struct can_frame *)arg;
        DEBUG("frame to send ID = 0x%08lx\n", frame->can_id);
        break;
    case CANDEV_EVENT_TX_ERROR:
        DEBUG("_can_event: CANDEV_EVENT_TX_ERROR\n");
        break;
    case CANDEV_EVENT_RX_INDICATION:
        DEBUG("_can_event: CANDEV_EVENT_RX_INDICATION\n");
        DEBUG("frame ID = 0x%08lx\n", frame->can_id);
        DEBUG("frame data = 0x");
        for (uint8_t i = 0; i < frame->can_dlc; i++) {
            DEBUG("%02x", frame->data[i]);
        }
        DEBUG("\n");
        dev->driver->send(candev, &frame_1);
        break;
    case CANDEV_EVENT_RX_ERROR:
        DEBUG("_can_event: CANDEV_EVENT_RX_ERROR\n");
        break;
    default:
        DEBUG("_can_event: unknown event\n");
        break;
    }
}

int main(void)
{
    puts("candev test application\n");

    gpio_init(GPIO_PIN(PC, 13), GPIO_IN);
    can_init(&can_samd5x, &candev_conf[1]);

    candev = &(can_samd5x.candev);
    candev->event_callback = _can_event_callback;
    candev->isr_arg = NULL;

    candev->driver->init(candev);

    struct can_filter filter = {0};
    filter.can_filter_conf = CAN_FILTER_RX_FIFO_1;
    filter.can_filter_type = CAN_FILTER_TYPE_CLASSIC;
    filter.can_id = 0x0111;
    filter.can_mask = 0x7FF;
    candev->driver->set_filter(candev, &filter);
    candev->driver->remove_filter(candev, &filter);

    filter.can_filter_conf = CAN_FILTER_RX_FIFO_1;
    filter.can_filter_type = CAN_FILTER_TYPE_CLASSIC;
    filter.can_id = 0x113;
    filter.can_mask = 0x7FF;
    candev->driver->set_filter(candev, &filter);

    filter.can_filter_conf = CAN_FILTER_RX_FIFO_0;
    filter.can_filter_type = CAN_FILTER_TYPE_CLASSIC;
    filter.can_id = 0x8C000001;
    filter.can_mask = 0x9FFFFFFE;
    candev->driver->set_filter(candev, &filter);

    filter.can_filter_conf = CAN_FILTER_RX_FIFO_0;
    filter.can_filter_type = CAN_FILTER_TYPE_CLASSIC;
    filter.can_id = 0x112;
    filter.can_mask = 0x7FF;
    candev->driver->set_filter(candev, &filter);

    /* Removing a non existing filter returns an error */
    struct can_filter filter_to_remove = {0};
    filter_to_remove.can_id = 0x114;
    filter_to_remove.can_mask = 0x7FF;
    candev->driver->remove_filter(candev, &filter_to_remove);

#if IS_ACTIVE(SAMD5X_CAN_TEST_MODE)
    candev_samd5x_enter_test_mode(candev);
#endif

    struct can_frame frame_1 = {
        .can_dlc = 5,
        .can_id = 0x8C122330,
        .data = {
            0,
            1,
            2,
            3,
            0xF
        }
    };

        struct can_frame frame_2 = {
        .can_dlc = 5,
        .can_id = 0x9B001122,
        .data = {
            4,
            5,
            6,
            7,
            0xE
        }
    };

#if IS_ACTIVE(TEST_MODE)
    candev_samd5x_enter_test_mode(candev);
#endif

    candev->driver->send(candev, &frame_1);
    candev->driver->send(candev, &frame_2);

    while(1);

    return 0;
}
