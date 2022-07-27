/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "log.h"
#include "slipdev.h"
#include "slipdev_internal.h"
#include "net/eui_provider.h"

/* XXX: BE CAREFUL ABOUT USING OUTPUT WITH MODULE_SLIPDEV_STDIO IN SENDING
 * FUNCTIONALITY! MIGHT CAUSE DEADLOCK!!!1!! */
#define ENABLE_DEBUG 0
#include "debug.h"

#include "isrpipe.h"
#include "mutex.h"
#include "stdio_uart.h"

static inline void slipdev_lock(void)
{
    if (IS_USED(MODULE_SLIPDEV_STDIO)) {
        mutex_lock(&slipdev_mutex);
    }
}

static inline void slipdev_unlock(void)
{
    if (IS_USED(MODULE_SLIPDEV_STDIO)) {
        mutex_unlock(&slipdev_mutex);
    }
}

static void _slip_rx_cb(void *arg, uint8_t byte)
{
    slipdev_t *dev = arg;

    switch (dev->state) {
#if IS_USED(MODULE_SLIPDEV_STDIO)
    case SLIPDEV_STATE_STDIN:
        switch (byte) {
        case SLIPDEV_ESC:
            dev->state = SLIPDEV_STATE_STDIN_ESC;
            break;
        case SLIPDEV_END:
            dev->state = SLIPDEV_STATE_NONE;
            byte = 0;
            /* fall-through */
        default:
            isrpipe_write_one(&slipdev_stdio_isrpipe, byte);
            break;
        }
        return;
    case SLIPDEV_STATE_STDIN_ESC:
        switch (byte) {
        case SLIPDEV_END_ESC:
            byte = SLIPDEV_END;
            break;
        case SLIPDEV_ESC_ESC:
            byte = SLIPDEV_ESC;
            break;
        }
        dev->state = SLIPDEV_STATE_STDIN;
        isrpipe_write_one(&slipdev_stdio_isrpipe, byte);
        return;
#endif
    case SLIPDEV_STATE_NONE:
        /* is diagnostic frame? */
        if (IS_USED(MODULE_SLIPDEV_STDIO) &&
            (byte == SLIPDEV_STDIO_START) &&
            (dev->config.uart == STDIO_UART_DEV)) {
            dev->state = SLIPDEV_STATE_STDIN;
            return;
        }

        /* ignore empty frame */
        if (byte == SLIPDEV_END) {
            return;
        }

        /* try to create new frame */
        if (!crb_start_chunk(&dev->rb)) {
            return;
        }
        dev->state = SLIPDEV_STATE_NET;
        /* fall-through */
    case SLIPDEV_STATE_NET:
        switch (byte) {
        case SLIPDEV_ESC:
            dev->state = SLIPDEV_STATE_NET_ESC;
            return;
        case SLIPDEV_END:
            crb_end_chunk(&dev->rb, true);
            netdev_trigger_event_isr(&dev->netdev);
            dev->state = SLIPDEV_STATE_NONE;
            return;
        }
        break;
    /* escaped byte received */
    case SLIPDEV_STATE_NET_ESC:
        switch (byte) {
        case SLIPDEV_END_ESC:
            byte = SLIPDEV_END;
            break;
        case SLIPDEV_ESC_ESC:
            byte = SLIPDEV_ESC;
            break;
        }
        dev->state = SLIPDEV_STATE_NET;
        break;
    }

    assert(dev->state == SLIPDEV_STATE_NET);

    /* discard frame if byte can't be added */
    if (!crb_add_byte(&dev->rb, byte)) {
        crb_end_chunk(&dev->rb, false);
        dev->state = SLIPDEV_STATE_NONE;
        return;
    }
}

static int _init(netdev_t *netdev)
{
    slipdev_t *dev = (slipdev_t *)netdev;

    DEBUG("slipdev: initializing device %p on UART %i with baudrate %" PRIu32 "\n",
          (void *)dev, dev->config.uart, dev->config.baudrate);
    /* initialize buffers */
    crb_init(&dev->rb, dev->rxmem, sizeof(dev->rxmem));
    if (uart_init(dev->config.uart, dev->config.baudrate, _slip_rx_cb,
                  dev) != UART_OK) {
        LOG_ERROR("slipdev: error initializing UART %i with baudrate %" PRIu32 "\n",
                  dev->config.uart, dev->config.baudrate);
        return -ENODEV;
    }
    return 0;
}

void slipdev_write_bytes(uart_t uart, const uint8_t *data, size_t len)
{
    for (unsigned j = 0; j < len; j++, data++) {
        switch (*data) {
            case SLIPDEV_END:
                /* escaping END byte*/
                slipdev_write_byte(uart, SLIPDEV_ESC);
                slipdev_write_byte(uart, SLIPDEV_END_ESC);
                break;
            case SLIPDEV_ESC:
                /* escaping ESC byte*/
                slipdev_write_byte(uart, SLIPDEV_ESC);
                slipdev_write_byte(uart, SLIPDEV_ESC_ESC);
                break;
            default:
                slipdev_write_byte(uart, *data);
        }
    }
}

static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    slipdev_t *dev = (slipdev_t *)netdev;
    int bytes = 0;

    DEBUG("slipdev: sending iolist\n");
    slipdev_lock();
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {
        uint8_t *data = iol->iol_base;
        slipdev_write_bytes(dev->config.uart, data, iol->iol_len);
        bytes += iol->iol_len;
    }
    slipdev_write_byte(dev->config.uart, SLIPDEV_END);
    slipdev_unlock();
    return bytes;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    slipdev_t *dev = (slipdev_t *)netdev;
    int res = 0;

    (void)info;
    if (buf == NULL) {
        if (len > 0) {
            /* remove data */
            crb_consume_chunk(&dev->rb, NULL, len);
        } else {
            /* the user was warned not to use a buffer size > `INT_MAX` ;-) */
            crb_get_chunk_size(&dev->rb, (size_t*)&res);
        }
    }
    else {
        crb_consume_chunk(&dev->rb, buf, len);
        res = len;
    }
    return res;
}

static void _isr(netdev_t *netdev)
{
    slipdev_t *dev = (slipdev_t *)netdev;

    DEBUG("slipdev: handling ISR event\n");

    size_t len;
    while (crb_get_chunk_size(&dev->rb, &len)) {
        DEBUG("slipdev: event handler set, issuing RX_COMPLETE event\n");
        netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
    }
}

static int _get(netdev_t *netdev, netopt_t opt, void *value, size_t max_len)
{
    (void)netdev;
    (void)value;
    (void)max_len;
    switch (opt) {
        case NETOPT_IS_WIRED:
            return 1;
        case NETOPT_DEVICE_TYPE:
            assert(max_len == sizeof(uint16_t));
            *((uint16_t *)value) = NETDEV_TYPE_SLIP;
            return sizeof(uint16_t);
#if IS_USED(MODULE_SLIPDEV_L2ADDR)
        case NETOPT_ADDRESS_LONG:
            assert(max_len == sizeof(eui64_t));
            netdev_eui64_get(netdev, value);
            return sizeof(eui64_t);
#endif
        default:
            return -ENOTSUP;
    }
}

static const netdev_driver_t slip_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = netdev_set_notsup,
};

void slipdev_setup(slipdev_t *dev, const slipdev_params_t *params, uint8_t index)
{
    /* set device descriptor fields */
    dev->config = *params;
    dev->state = 0;
    dev->netdev.driver = &slip_driver;

    netdev_register(&dev->netdev, NETDEV_SLIPDEV, index);
}

/** @} */
