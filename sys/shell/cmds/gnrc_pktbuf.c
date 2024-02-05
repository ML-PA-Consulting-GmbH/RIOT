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
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 */

#include <stdio.h>

#include "net/gnrc/pktbuf.h"
#include "shell.h"

static int _gnrc_pktbuf_cmd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    gnrc_pktbuf_stats();
    puts("");
#if IS_USED(MODULE_GNRC_TRACE_PKTBUF_STATIC)
    gnrc_pktbuf_print_leases();
    printf("pktbuf: leased out: %"PRIu32"\n", gnrc_pktbuf_get_usage());
#endif
    return 0;
}

SHELL_COMMAND(pktbuf, "prints internal stats of the packet buffer", _gnrc_pktbuf_cmd);

/** @} */
