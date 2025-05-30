/*
 * Copyright (C) 2016 Alexander Aring <aar@pengutronix.de>
 *                    Freie Universität Berlin
 *                    HAW Hamburg
 *                    Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    net_sock    Sock API
 * @ingroup     net
 * @brief       Provides a network API for applications and library
 *
 * About
 * =====
 *
 * ~~~~~~~~~~~~~~~~~~~~
 *    +---------------+
 *    |  Application  |
 *    +---------------+
 *            ^
 *            |
 *            v
 *          sock
 *            ^
 *            |
 *            v
 *    +---------------+
 *    | Network Stack |
 *    +---------------+
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * This module provides a set of functions to establish connections or send and
 * receive datagrams using different types of protocols. Together, they serve as
 * an API that allows an application or library to connect to a network.
 *
 * It was designed with the following priorities in mind
 *
 * 1. No need for dynamic memory allocation
 * 2. User friendliness
 * 3. Simplicity
 * 4. Efficiency (at both front- and backend)
 * 5. Portability
 *
 * Currently the following `sock` types are defined:
 *
 * * @ref sock_ip_t (net/sock/ip.h): raw IP sock
 * * @ref sock_tcp_t (net/sock/tcp.h): TCP sock
 * * @ref sock_udp_t (net/sock/udp.h): UDP sock
 * * @ref sock_dtls_t (net/sock/dtls.h): DTLS sock
 *
 * Note that there might be no relation between the different `sock` types.
 * So casting e.g. `sock_ip_t` to `sock_udp_t` might not be as straight forward,
 * as you think depending on the networking architecture.
 *
 * How To Use
 * ==========
 *
 * A RIOT application uses the functions provided by one or more of the
 * `sock` type headers (for example @ref sock_udp_t), regardless of the
 * network stack it uses.
 * The network stack used under the bonnet is specified by including the
 * appropriate module (for example `USEMODULE += gnrc_sock_udp` for
 * [GNRC's](@ref net_gnrc) version of this API).
 *
 * This allows for network stack agnostic code on the application layer.
 * The application code to establish a connection is always the same, allowing
 * the network stack underneath to be switched simply by changing the
 * `USEMODULE` definitions in the application's Makefile.
 *
 * The actual code very much depends on the used `sock` type. Please refer to
 * their documentation for specific examples.
 *
 * Implementer Notes
 * =================
 * ### Type definition
 * For simplicity and modularity this API doesn't put any restriction on the
 * actual implementation of the type. For example, one implementation might
 * choose to have all `sock` types having a common base class or use the raw IP
 * `sock` type to send e.g. UDP packets, while others will keep them
 * completely separate from each other.
 *
 * @author  Alexander Aring <aar@pengutronix.de>
 * @author  Simon Brummer <simon.brummer@haw-hamburg.de>
 * @author  Cenk Gündoğan <mail@cgundogan.de>
 * @author  Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @{
 *
 * @file
 * @brief   Common sock API definitions
 *
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 */

#include <stdint.h>
#include <stddef.h>
#include "iolist.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DOXYGEN)
/**
 * @name        Compile flags
 * @brief       Flags to (de)activate certain functionalities
 * @{
 */
#define SOCK_HAS_IPV6       /**< activate IPv6 support */
/**
 * @brief   activate asynchronous event functionality
 *
 * @see @ref net_sock_async
 */
#define SOCK_HAS_ASYNC
/**
 * @brief   Activate context for asynchronous events
 *
 * @see @ref net_sock_async
 *
 * This can be used if an asynchronous mechanism needs context (e.g. an
 * event instance for an event loop). An event handling implementation then
 * needs to provide a `sock_async_ctx.h` header file containing a definition
 * for the `sock_async_ctx_t` type.
 */
#define SOCK_HAS_ASYNC_CTX
/** @} */
#endif

/**
 * @name        Sock flags
 * @brief       Common flags for @ref net_sock
 * @anchor      net_sock_flags
 * @{
 */
#define SOCK_FLAGS_REUSE_EP         (0x0001)    /**< allow to reuse end point on bind */
#define SOCK_FLAGS_CONNECT_REMOTE   (0x0002)    /**< restrict responses to remote address */
/** @} */

/**
 * @brief   Special netif ID for "any interface"
 * @todo    Use an equivalent definition from PR #5511
 */
#define SOCK_ADDR_ANY_NETIF (0)

/**
 * @brief   Address to bind to any IPv4 address
 */
#define SOCK_IPV4_EP_ANY        { .family = AF_INET, \
                                  .netif = SOCK_ADDR_ANY_NETIF }

#if defined(SOCK_HAS_IPV6) || defined(DOXYGEN)
/**
 * @brief   Address to bind to any IPv6 address
 */
#define SOCK_IPV6_EP_ANY        { .family = AF_INET6, \
                                  .netif = SOCK_ADDR_ANY_NETIF }
#endif

/**
 * @brief   Special value meaning "wait forever" (don't timeout)
 */
#define SOCK_NO_TIMEOUT     (UINT32_MAX)

/**
 * @brief   Abstract IP end point and end point for a raw IP sock object
 */
typedef struct {
    /**
     * @brief family of sock_ip_ep_t::addr
     *
     * @see @ref net_af
     */
    int family;

    union {
#ifdef SOCK_HAS_IPV6
        /**
         * @brief IPv6 address mode
         *
         * @note only available if @ref SOCK_HAS_IPV6 is defined.
         */
        uint8_t ipv6[16];
#endif
        uint8_t ipv4[4];        /**< IPv4 address mode */
        uint32_t ipv4_u32;      /**< IPv4 address *in network byte order* */
    } addr;                 /**< address */

    /**
     * @brief   stack-specific network interface ID
     *
     * @todo    port to common network interface identifiers in PR #5511.
     *
     * Use @ref SOCK_ADDR_ANY_NETIF for any interface.
     * For reception this is the local interface the message came over,
     * for transmission, this is the local interface the message should be send
     * over
     */
    uint16_t netif;
} sock_ip_ep_t;

/**
 * @brief   Common IP-based transport layer end point
 */
struct _sock_tl_ep {
    /**
     * @brief family of sock_ip_ep_t::addr
     *
     * @see @ref net_af
     */
    int family;

    union {
#ifdef SOCK_HAS_IPV6
        /**
         * @brief IPv6 address mode
         *
         * @note only available if @ref SOCK_HAS_IPV6 is defined.
         */
        uint8_t ipv6[16];
#endif
        uint8_t ipv4[4];        /**< IPv4 address mode */
        uint32_t ipv4_u32;      /**< IPv4 address *in network byte order* */
    } addr;                 /**< address */

    /**
     * @brief   stack-specific network interface ID
     *
     * @todo    port to common network interface identifiers in PR #5511.
     *
     * Use @ref SOCK_ADDR_ANY_NETIF for any interface.
     * For reception this is the local interface the message came over,
     * for transmission, this is the local interface the message should be send
     * over
     */
    uint16_t netif;
    uint16_t port;          /**< transport layer port (in host byte order) */
};

/**
 * @brief   Flags used to request auxiliary data
 */
enum {
    /**
     * @brief   Flag to request the local address/endpoint
     *
     * @note    Select module `sock_aux_local` and a compatible network stack
     *          to use this
     *
     * This is the address/endpoint the packet/datagram/segment was received on.
     * This flag will be cleared if the network stack stored the local
     * address/endpoint as requested, otherwise the bit remains set.
     *
     * Depending on the family of the socket, the timestamp will be stored in
     * @ref sock_udp_aux_rx_t::local, @ref sock_ip_aux_rx_t::local, or in
     * @ref sock_dtls_aux_rx_t::local.
     */
    SOCK_AUX_GET_LOCAL = (1LU << 0),
    /**
     * @brief   Flag to request the time stamp of transmission / reception
     *
     * @note    Select module `sock_aux_timestamp` and a compatible network
     *          stack to use this
     *
     * Unless otherwise noted, the time stamp is the current system time in
     * nanoseconds on which the start of frame delimiter or preamble was
     * sent / received.
     *
     * Set this flag in the auxiliary data structure prior to the call of
     * @ref sock_udp_recv_aux / @ref sock_udp_send_aux / @ref sock_ip_recv_aux
     * / etc. to request the time stamp of reception / transmission. This flag
     * will be cleared if the timestamp was stored, otherwise it remains set.
     *
     * Depending on the family of the socket, the timestamp will be stored in
     * for reception in @ref sock_udp_aux_rx_t::timestamp,
     * @ref sock_ip_aux_rx_t::timestamp, or @ref sock_dtls_aux_rx_t::timestamp.
     * For transmission it will be stored in @ref sock_udp_aux_tx_t::timestamp,
     * @ref sock_ip_aux_tx_t::timestamp, or @ref sock_dtls_aux_tx_t::timestamp.
     */
    SOCK_AUX_GET_TIMESTAMP = (1LU << 1),
    /**
     * @brief   Flag to request the RSSI value of received frame
     *
     * @note    Select module `sock_aux_rssi` and a compatible network stack to
     *          use this
     *
     * Set this flag in the auxiliary data structure prior to the call of
     * @ref sock_udp_recv_aux / @ref sock_ip_recv_aux / etc. to request the
     * RSSI value of a received frame. This flag will be cleared if the
     * timestamp was stored, otherwise it remains set.
     *
     * Depending on the family of the socket, the RSSI value will be stored in
     * @ref sock_udp_aux_rx_t::rssi, @ref sock_ip_aux_rx_t::rssi, or
     * @ref sock_dtls_aux_rx_t::rssi.
     */
    SOCK_AUX_GET_RSSI = (1LU << 2),
    /**
     * @brief   Flag to set the local address/endpoint
     *
     * @note    Select module `sock_aux_local` and a compatible network stack
     *          to use this
     *
     * This is the address/endpoint the packet/datagram/segment will be sent from.
     * This flag will be cleared if the network stack stored the local
     * address/endpoint as requested, otherwise the bit remains set.
     *
     * Depending on the family of the socket, the timestamp will be stored in
     * @ref sock_udp_aux_tx_t::local, @ref sock_ip_aux_tx_t::local, or in
     * @ref sock_dtls_aux_tx_t::local.
     */
    SOCK_AUX_SET_LOCAL = (1LU << 3),
    /**
     * @brief   Flag to request the TTL value of received frame
     *
     * @note    Select module `sock_aux_ttl` and a compatible network stack to
     *          use this
     *
     * Set this flag in the auxiliary data structure prior to the call of
     * @ref sock_udp_recv_aux / @ref sock_ip_recv_aux / etc. to request the
     * TTL value of a received frame. This flag will be cleared if the
     * time to live was stored, otherwise it remains set.
     *
     * Depending on the family of the socket, the TTL value will be stored in
     * @ref sock_udp_aux_rx_t::ttl or @ref sock_dtls_aux_rx_t::ttl.
     */
    SOCK_AUX_GET_TTL = (1LU << 4),
};

/**
 * @brief   Type holding the flags used to request specific auxiliary data
 *
 * This is a bitmask of `SOCK_AUX_GET_...`, e.g. if the mask contains
 * @ref SOCK_AUX_GET_LOCAL, the local address/endpoint is requested
 *
 * @details The underlying type can be changed without further notice, if more
 *          flags are needed. Thus, only the `typedef`ed type should be used
 *          to store the flags.
 */
typedef uint8_t sock_aux_flags_t;

#ifdef __cplusplus
}
#endif

/** @} */
