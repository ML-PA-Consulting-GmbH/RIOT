/*
 * Copyright (C) 2021 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @ingroup     test_utils_benchmark_udp
 * @{
 *
 * @file
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#include <stdio.h>
#include "net/sock/udp.h"
#include "net/utils.h"
#include "xtimer.h"

#include "test_utils/benchmark_udp.h"

static sock_udp_t sock;
static uint32_t delay_us = US_PER_SEC;
static uint16_t payload_size = 32;

static char send_thread_stack[THREAD_STACKSIZE_DEFAULT];
static char listen_thread_stack[THREAD_STACKSIZE_DEFAULT];

static uint8_t buf_tx[BENCH_PAYLOAD_SIZE_MAX];
static benchmark_msg_ping_t *ping = (void *)buf_tx;

static bool running;

struct {
    uint32_t seq_no;
    uint32_t time_tx_us;
} record_tx[4];

static uint32_t _get_rtt(uint32_t seq_num, uint32_t prev)
{
    for (unsigned i = 0; i < ARRAY_SIZE(record_tx); ++i) {
        if (record_tx[i].seq_no == seq_num) {
            return xtimer_now_usec() - record_tx[i].time_tx_us;
        }
    }

    return prev;
}

static void _put_rtt(uint32_t seq_num) {
    uint8_t oldest = 0;
    uint32_t oldest_diff = 0;
    uint32_t now = xtimer_now_usec();

    for (unsigned i = 0; i < ARRAY_SIZE(record_tx); ++i) {
        uint32_t diff = now - record_tx[i].time_tx_us;
        if (diff > oldest_diff) {
            oldest_diff = diff;
            oldest = i;
        }
    }

    record_tx[oldest].seq_no = seq_num;
    record_tx[oldest].time_tx_us = now;
}

static void *_listen_thread(void *ctx)
{
    (void)ctx;

    static uint8_t buf[BENCH_PAYLOAD_SIZE_MAX];
    benchmark_msg_cmd_t *cmd = (void *)buf;

    while (running) {
        ssize_t res;

        res = sock_udp_recv(&sock, buf, sizeof(buf), 2 * delay_us, NULL);
        if (res < 0) {
            if (res != -ETIMEDOUT) {
                printf("Error receiving message: %zd\n", res);
            }
            continue;
        }

        unsigned state = irq_disable();
        if (cmd->flags & BENCH_FLAG_CMD_PKT) {
            ping->seq_no  = 0;
            ping->replies = 0;
            ping->flags   = cmd->flags & BENCH_MASK_COOKIE;
            delay_us      = cmd->delay_us;
            payload_size  = cmd->payload_len;
        } else {
            benchmark_msg_ping_t *pong = (void *)buf;

            ping->replies++;
            ping->rtt_last = _get_rtt(pong->seq_no, ping->rtt_last);
        }
        irq_restore(state);
    }

    sock_udp_close(&sock);

    return NULL;
}

static void *_send_thread(void *ctx)
{
    sock_udp_ep_t remote = *(sock_udp_ep_t*)ctx;

    while (running) {
        _put_rtt(ping->seq_no);

        if (sock_udp_send(&sock, ping, sizeof(*ping) + payload_size, &remote) < 0) {
            puts("Error sending message");
            continue;
        } else {
            unsigned state = irq_disable();
            ping->seq_no++;
            irq_restore(state);
        }

        xtimer_usleep(delay_us);
    }

    return NULL;
}

int benchmark_udp_start(const char *server, uint16_t port)
{
    netif_t *netif;
    sock_udp_ep_t local = { .family = AF_INET6,
                            .netif = SOCK_ADDR_ANY_NETIF,
                            .port = port };
    sock_udp_ep_t remote = { .family = AF_INET6,
                             .port = port };

    /* stop threads first */
    if (running) {
        running = false;
        xtimer_usleep(delay_us * 2);
    }

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Error creating UDP sock");
        return 1;
    }

    if (netutils_get_ipv6((ipv6_addr_t *)&remote.addr.ipv6, &netif, server) < 0) {
        puts("can't resolve remote address");
        return 1;
    }
    if (netif) {
        remote.netif = netif_get_id(netif);
    } else {
        remote.netif = SOCK_ADDR_ANY_NETIF;
    }

    running = true;
    thread_create(listen_thread_stack, sizeof(listen_thread_stack),
                  THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST,
                  _listen_thread, NULL, "UDP receiver");
    thread_create(send_thread_stack, sizeof(send_thread_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                  _send_thread, &remote, "UDP sender");
    return 0;
}

bool benchmark_udp_stop(void)
{
    bool stopped = running;
    running = false;
    return stopped;
}
/** @} */
