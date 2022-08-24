/*
 * Copyright (C) 2022 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_nanocoap
 * @{
 *
 * @file
 * @brief       NanoCoAP Link Format parser
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 *
 * @}
 */

#include "net/nanocoap/link_format.h"
#include "net/nanocoap_sock.h"
#include "net/sock/util.h"

struct dir_list_ctx {
    char *buf;
    char *cur;
    char *end;
    coap_link_format_handler_t cb;
    void *ctx;
};

static int _dirlist_cb(void *arg, size_t offset, uint8_t *buf, size_t len, int more)
{
    (void)offset;
    (void)more;

    struct dir_list_ctx *ctx = arg;

    char *end = (char *)buf + len;
    for (char *c = (char *)buf; c < end; ++c) {
        if (ctx->cur) {
            if (*c == '>' || ctx->cur == ctx->end) {
                int res;
                *ctx->cur = 0;
                res = ctx->cb(ctx->buf, ctx->ctx);
                ctx->cur = NULL;
                if (res < 0) {
                    return res;
                }
            } else {
                *ctx->cur++ = *c;
            }
        } else if (*c == '<') {
            ctx->cur = ctx->buf;
        }
    }

    return 0;
}

int nanocoap_link_format_get(nanocoap_sock_t *sock, const char *path,
                    coap_link_format_handler_t cb, void *arg)
{
    char buffer[CONFIG_NANOCOAP_QS_MAX];
    struct dir_list_ctx ctx = {
        .buf = buffer,
        .end = buffer + sizeof(buffer),
        .cb = cb,
        .ctx = arg,
    };
    return nanocoap_sock_get_blockwise(sock, path, CONFIG_NANOCOAP_BLOCKSIZE_DEFAULT,
                                       _dirlist_cb, &ctx);
}

int nanocoap_link_format_get_url(const char *url, coap_link_format_handler_t cb, void *arg)
{
    nanocoap_sock_t sock;
    int res = nanocoap_sock_url_connect(url, &sock);
    if (res) {
        return res;
    }

    res = nanocoap_link_format_get(&sock, sock_urlpath(url), cb, arg);
    nanocoap_sock_close(&sock);

    return res;
}
