/*
 * Copyright (C) 2021 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#include "net/gnrc/ipv6/nib.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

void gnrc_nib_rtr_adv_pio_cb(gnrc_netif_t *upstream, const ndp_opt_pi_t *pio)
{
    gnrc_netif_t *downstream = NULL;
    const ipv6_addr_t *prefix = &pio->prefix;
    uint32_t valid_ltime = byteorder_ntohl(pio->valid_ltime);
    uint32_t pref_ltime = byteorder_ntohl(pio->pref_ltime);

    /* create a subnet for each downstream interface */
    unsigned subnets = gnrc_netif_numof() - 1;

    const uint8_t prefix_len = pio->prefix_len;
    uint8_t new_prefix_len;

    if (subnets == 0) {
        return;
    }

    new_prefix_len = prefix_len + 32 - __builtin_clz(subnets);

    if (new_prefix_len > 64) {
        DEBUG("nib: can't split /%u into %u subnets\n", prefix_len, subnets);
        return;
    }

    while ((downstream = gnrc_netif_iter(downstream))) {
        ipv6_addr_t new_prefix;

        if (downstream == upstream) {
            continue;
        }

        /* create subnet by adding interface index */
        new_prefix.u64[0].u64 = byteorder_ntohll(prefix->u64[0]);
        new_prefix.u64[0].u64 |= (uint64_t)subnets-- << (63 - prefix_len);
        new_prefix.u64[0] = byteorder_htonll(new_prefix.u64[0].u64);

        DEBUG("nib: configure prefix %s/%u on %u\n",
              ipv6_addr_to_str(addr_str, &new_prefix, sizeof(addr_str)),
              new_prefix_len, downstream->pid);

        gnrc_netif_ipv6_add_prefix(downstream, &new_prefix, new_prefix_len,
                                   valid_ltime, pref_ltime);

        /* start advertising subnet */
        gnrc_ipv6_nib_change_rtr_adv_iface(downstream, true);
    }

    /* Disable router advertisements on upstream interface.
     * 1. Does not confuse the upstream router to add the border router to its
     *    default router list and
     * 2. Includes Route Information Option in last router advertisement to
     *    inform upstream router about downstream subnets.
     */
    gnrc_ipv6_nib_send_final_rtr_adv(upstream);
}
/** @} */
