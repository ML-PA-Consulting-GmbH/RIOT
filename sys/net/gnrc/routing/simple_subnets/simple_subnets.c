/*
 * Copyright (C) 2021 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_simple_subnets Simple-Subnet auto-configuration
 * @ingroup     net_gnrc
 * @brief       Automatic configuration for cascading subnets
 *
 * About
 * =====
 *
 * This module provides an automatic configuration for networks with a simple
 * tree tolpology.
 *
 * If a sufficiently large IPv6 prefix is provided via Router Advertisements, a
 * routing node with this module will automatically configure subnets from it for
 * each downstream interface.
 *
 * There can only be a single routing node on each level of the network but an
 * arbitrary number of leaf nodes.
 *
 * ![Example Topology](simple_subnets.svg)
 *
 * The downstream network(s) get the reduced prefix via Router Advertisements and
 * the process repeats until the bits of prefix are exhausted. (The smallest
 * must still have a /64 prefix.)
 *
 * The downstream router will send an otherwise empty router advertisement with only
 * the Route Information Option set to the upstream network.
 * The Route Information Option contains the prefix of the downstream network so that
 * upstream hosts will no longer consider hosts in this subnet on-link but instead
 * will use the downstream router to route to the new subnet.
 *
 * Usage
 * =====
 *
 * Simply add the `gnrc_simple_subnets` module to the nodes that should act as routers
 * in the cascading network.
 * The upstream network will be automatically chosen as the one that first receives a
 * router advertisement.
 *
 * @{
 *
 * @file
 * @author  Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ndp.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

void gnrc_ipv6_nib_rtr_adv_pio_cb(gnrc_netif_t *upstream, const ndp_opt_pi_t *pio)
{
    gnrc_netif_t *downstream = NULL;
    gnrc_pktsnip_t *ext_opts = NULL;
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
        DEBUG("simple_subnets: can't split /%u into %u subnets\n", prefix_len, subnets);
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

        DEBUG("simple_subnets: configure prefix %s/%u on %u\n",
              ipv6_addr_to_str(addr_str, &new_prefix, sizeof(addr_str)),
              new_prefix_len, downstream->pid);

        gnrc_netif_ipv6_add_prefix(downstream, &new_prefix, new_prefix_len,
                                   valid_ltime, pref_ltime);

        /* start advertising subnet */
        gnrc_ipv6_nib_change_rtr_adv_iface(downstream, true);

        /* add route information option with new subnet */
        ext_opts = gnrc_ndp_opt_ri_build(&new_prefix, new_prefix_len, valid_ltime,
                                         NDP_OPT_RI_FLAGS_PRF_NONE, ext_opts);
        if (ext_opts == NULL) {
            DEBUG("simple_subnets: No space left in packet buffer. Not adding RIO\n");
        }
    }

    /* immediately send an RA with RIO */
    gnrc_ndp_rtr_adv_send(upstream, NULL, &ipv6_addr_all_nodes_link_local, true, ext_opts);
}
/** @} */
