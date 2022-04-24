/*
 * Copyright (C) 2022 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_nanosock
 * @brief       NanoCoAP Resource Directory helper functions
 *
 * @{
 *
 * @file
 * @brief       NanoCoAP Resource Directory helper functions
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */
#ifndef NET_NANOCOAP_RD_H
#define NET_NANOCOAP_RD_H

#include "net/nanocoap_sock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Callback function called for each resource on the directory
 *
 * @param[in]   path    Resource path from the server
 * @param[in]   ctx     Optional function context
 *
 * @returns     0 on success
 * @returns     <0 on error
 */
typedef int (*coap_rd_handler_t)(char *path, void *ctx);

/**
 * @brief   Downloads the resource behind @p path via blockwise GET
 *
 * @param[in]   sock    Connection to the server
 * @param[in]   path    path of the resource
 * @param[in]   cb      Callback to execute for each resource entry
 * @param[in]   arg     Optional callback argument
 *
 * @returns     0 on success
 * @returns     <0 on error
 */
int nanocoap_rd_get(nanocoap_sock_t *sock, const char *path,
                    coap_rd_handler_t cb, void *arg);

/**
 * @brief   Downloads the resource behind @p url via blockwise GET
 *
 * @param[in]   url     URL to the resource
 * @param[in]   cb      Callback to execute for each resource entry
 * @param[in]   arg     Optional callback argument
 *
 * @returns     0 on success
 * @returns     <0 on error
 */
int nanocoap_rd_get_url(const char *url, coap_rd_handler_t cb, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* NET_NANOCOAP_RD_H */
/** @} */
