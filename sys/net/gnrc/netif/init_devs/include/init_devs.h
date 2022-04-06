/*
 * Copyright (C) 2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_auto_init_gnrc_netif
 * @{
 *
 * @file
 * @brief       common netif device initialization definitions
 *
 * @author      Fabian Hüßler <fabian.huessler@ovgu.de>
 */

#ifndef INIT_DEVS_H
#define INIT_DEVS_H

#include "thread.h"
#include "msg.h"
#include "net/gnrc/netif/conf.h"    /* <- GNRC_NETIF_MSG_QUEUE_SIZE */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GNRC_NETIF_EXTRA_STACKSIZE
/**
 * @brief   Additional stack size required by the driver
 *
 * With increasing of GNRC_NETIF_MSG_QUEUE_SIZE the required stack size
 * increases as well. A queue size of 8 messages works with default stack size,
 * so we increase the stack by `sizeof(msg_t)` for each additional element
 */
#define GNRC_NETIF_EXTRA_STACKSIZE      ((GNRC_NETIF_MSG_QUEUE_SIZE - 8) * sizeof(msg_t))
#endif

/**
 * @brief   stack size of a netif thread
 */
#ifndef GNRC_NETIF_STACKSIZE_DEFAULT
#define GNRC_NETIF_STACKSIZE_DEFAULT    (THREAD_STACKSIZE_DEFAULT + \
                                         GNRC_NETIF_EXTRA_STACKSIZE)
#endif

/**
 * @brief   extra stack size if ieee802154 security is enabled
 *
 * You may increase this value if you experience a stack overflow
 * with IEEE 802.15.4 security enabled.
 */
#ifdef MODULE_IEEE802154_SECURITY
#define IEEE802154_SECURITY_EXTRA_STACKSIZE (128)
#else
#define IEEE802154_SECURITY_EXTRA_STACKSIZE (0)
#endif

#ifndef IEEE802154_STACKSIZE_DEFAULT
/**
 * @brief   stack size of an ieee802154 device
 */
#define IEEE802154_STACKSIZE_DEFAULT    (GNRC_NETIF_STACKSIZE_DEFAULT + \
                                         IEEE802154_SECURITY_EXTRA_STACKSIZE)
#endif

#ifdef __cplusplus
}
#endif

#endif /* INIT_DEVS_H */
/** @} */
