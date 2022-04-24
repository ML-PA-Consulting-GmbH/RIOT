/*
 * Copyright (C) 2020 chrysn
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
/**
 * @{
 *
 * @file
 * @brief       CoAP file server implementation
 *
 * @author      Christian Amsüss <chrysn@fsfe.org>
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "checksum/fletcher32.h"
#include "net/gcoap/fileserver.h"
#include "net/gcoap.h"
#include "vfs.h"

#define ENABLE_DEBUG 0
#include "debug.h"

/** Maximum length of an expressible path, including the trailing 0 character. */
#define COAPFILESERVER_PATH_MAX (64)

/** Data extracted from a request on a file */
struct requestdata {
    /** 0-terminated expanded file name in the VFS */
    char namebuf[COAPFILESERVER_PATH_MAX];
    uint32_t blocknum2;
    unsigned int szx2; /* would prefer uint8_t but that's what coap_get_blockopt gives */
    uint32_t etag;
    bool etag_sent;
};

/* These are almost but but not quite coap_handler_t -- they require the
 * request to be already fully read and digested into the last argument. */
static ssize_t coapfileserver_file_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                           struct requestdata *request);
static ssize_t coapfileserver_directory_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                                struct requestdata *request,
                                                struct coapfileserver_entry *entry);

/** Create a CoAP response for a given errno (eg. EACCESS -> 4.03 Forbidden
 * etc., defaulting to 5.03 Internal Server Error) */
static size_t coapfileserver_errno_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, int err);

/**
 * @brief  Return true if path/name is a directory.
 */
static bool entry_is_dir(char *path, const char *name)
{
    struct stat stat;
    size_t path_len = strlen(path);
    size_t name_len = strlen(name);

    if (path_len + name_len + 1 >= COAPFILESERVER_PATH_MAX) {
        return false;
    }

    /* re-use path buffer, it is already COAPFILESERVER_PATH_MAX bytes long */
    path[path_len] = '/';
    path[path_len + 1] = 0;
    strncat(path, name, COAPFILESERVER_PATH_MAX - 1);

    if (vfs_stat(path, &stat)) {
        DEBUG("vfs_stat(%s) failed\n", path);
    }

    path[path_len] = 0;
    return (stat.st_mode & S_IFMT) == S_IFDIR;
}

static unsigned _count_char(const char *s, char c)
{
    unsigned count = 0;
    while (*s) {
        if (*s == c) {
            ++count;
        }
        ++s;
    }

    return count;
}

static size_t _copy_string(void *dst, const void *src, size_t len_max)
{
    size_t len = strlen(src);
    if (len > len_max) {
        return 0;
    }
    memcpy(dst, src, len);
    return len;
}

/** Build an ETag based on the given file's VFS stat. If the stat fails,
 * returns the error and leaves etag in any state; otherwise there's an etag
 * in the stattag's field */
static int stat_etag(const char *filename, uint32_t *etag)
{
    struct stat stat;
    int err = vfs_stat(filename, &stat);
    if (err < 0) {
        return err;
    }

    /* Normalizing fields whose value can change without affecting the ETag */
    stat.st_nlink = 0;
    memset(&stat.st_atime, 0, sizeof(stat.st_atime));

    *etag = fletcher32((void *)&stat, sizeof(stat) / 2);

    return 0;
}

ssize_t coapfileserver_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx) {
    struct coapfileserver_entry *entry = ctx;
    struct requestdata request = {
        .etag_sent = false,
        .blocknum2 = 0,
        .szx2 = CONFIG_NANOCOAP_BLOCK_SIZE_EXP_MAX,
    };

    /** Index in request.namebuf. Must not point at the last entry as that will be
     * zeroed to get a 0-terminated string. */
    size_t namelength = 0;
    uint8_t errorcode = COAP_CODE_INTERNAL_SERVER_ERROR;
    uint8_t strip_remaining = _count_char(entry->resource, '/');

    /* If a root directory for the server was specified, use that */
    if (entry->root && strlen(entry->root) > 1) {
        strncpy(request.namebuf, entry->root, sizeof(request.namebuf));
        namelength = strlen(entry->root);
    }

    bool is_directory = true; /* either no path component at all or trailing '/' */
    coap_optpos_t opt = {
        .offset = coap_get_total_hdr_len(pdu),
    };
    uint8_t *value;
    ssize_t optlen;
    while ((optlen = coap_opt_get_next(pdu, &opt, &value, 0)) != -ENOENT) {

        if (optlen < 0) {
            errorcode = COAP_CODE_BAD_REQUEST;
            goto error;
        }

        switch (opt.opt_num) {
            case COAP_OPT_URI_PATH:
                if (strip_remaining != 0) {
                    strip_remaining -= 1;
                    continue;
                }
                if ((is_directory = (optlen == 0))) { /* '/' */
                    continue;
                }
                if (memchr(value, '\0', optlen) != NULL ||
                    memchr(value, '/', optlen) != NULL) {
                    /* Path can not be expressed in the file system */
                    errorcode = COAP_CODE_PATH_NOT_FOUND;
                    goto error;
                }
                size_t newlength = namelength + 1 + optlen;
                if (newlength > sizeof(request.namebuf) - 1) {
                    /* Path too long, therefore can't exist in this mapping */
                    errorcode = COAP_CODE_PATH_NOT_FOUND;
                    goto error;
                }
                request.namebuf[namelength] = '/';
                memcpy(&request.namebuf[namelength] + 1, value, optlen);
                namelength = newlength;
                break;
            case COAP_OPT_ETAG:
                if (optlen != sizeof(request.etag)) {
                    /* Can't be a matching tag, no use in carrying that */
                    continue;
                }
                if (request.etag_sent) {
                    /* We can reasonably only check for a limited sized set,
                     * and it size is 1 here (sending multiple ETags is
                     * possible but rare) */
                    continue;
                }
                request.etag_sent = true;
                memcpy(&request.etag, value, sizeof(request.etag));
                break;
            case COAP_OPT_BLOCK2:
                /* Could be more efficient now that we already know where it
                 * is, but meh */
                coap_get_blockopt(pdu, COAP_OPT_BLOCK2, &request.blocknum2, &request.szx2);
                break;
            default:
                if (opt.opt_num & 1) {
                    errorcode = COAP_CODE_BAD_REQUEST;
                    goto error;
                }
                else {
                    /* Ignoring elective option */
                }
        }
    }

    request.namebuf[namelength] = '\0';

    DEBUG("request: '%s'\n", request.namebuf);

    /* Note to self: As we parse more options than just Uri-Path, we'll likely
     * pass a struct pointer later. So far, those could even be hooked into the
     * resource list, but that'll go away once we parse more options */
    if (is_directory) {
        return coapfileserver_directory_handler(pdu, buf, len, &request, entry);
    }
    else {
        return coapfileserver_file_handler(pdu, buf, len, &request);
    }

error:
    return gcoap_response(pdu, buf, len, errorcode);
}

static ssize_t coapfileserver_file_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                           struct requestdata *request)
{
    uint32_t etag;
    int err = stat_etag(request->namebuf, &etag);
    if (err < 0) {
        return coapfileserver_errno_handler(pdu, buf, len, err);
    }

    if (request->etag_sent && etag == request->etag) {
        gcoap_resp_init(pdu, buf, len, COAP_CODE_VALID);
        coap_opt_add_opaque(pdu, COAP_OPT_ETAG, &etag, sizeof(etag));
        return coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
    }

    int fd = vfs_open(request->namebuf, O_RDONLY, 0);
    if (fd < 0) {
        return coapfileserver_errno_handler(pdu, buf, len, fd);
    }

    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_opaque(pdu, COAP_OPT_ETAG, &etag, sizeof(etag));
    /* To best see how this works set CONFIG_GCAOP_PDU_BUF_SIZE to 532 or 533.
     * If we did a sharper estimation (factoring in the block2 size option with
     * the current blockum), we'd even pack 512 bytes into 530 until block
     * numbers get large enough to eat another byte, which is when the block
     * size would decrease in-flight. */
    size_t remaining_length = len - (pdu->payload - buf);
    remaining_length -= 5; /* maximum block2 option usable in nanocoap */
    remaining_length -= 1; /* payload marker */
    /* > 0: To not wrap around; if that still won't fit that's later caught in
     * an assertion */
    while ((coap_szx2size(request->szx2) > remaining_length) && (request->szx2 > 0)) {
        request->szx2--;
        request->blocknum2 <<= 1;
    }
    coap_block_slicer_t slicer;
    coap_block_slicer_init(&slicer, request->blocknum2, coap_szx2size(request->szx2));
    coap_opt_add_block2(pdu, &slicer, true);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    err = vfs_lseek(fd, slicer.start, SEEK_SET);
    if (err < 0) {
        goto late_err;
    }

    /* That'd only happen if the buffer is too small for even a 16-byte block,
     * or if the above calculations were wrong.
     *
     * Not using payload_len here as that's needlessly underestimating the
     * space by CONFIG_GCOAP_RESP_OPTIONS_BUF
     * */
    assert(pdu->payload + slicer.end - slicer.start <= buf + len);
    int read = vfs_read(fd, pdu->payload, slicer.end - slicer.start);
    if (read < 0) {
        goto late_err;
    }

    uint8_t morebuf;
    int more = vfs_read(fd, &morebuf, 1);
    if (more < 0) {
        goto late_err;
    }

    vfs_close(fd);

    slicer.cur = slicer.end + more;
    coap_block2_finish(&slicer);

    if (read == 0) {
        /* Rewind to clear payload marker */
        read -= 1;
    }

    return resp_len + read;

late_err:
    vfs_close(fd);
    coap_hdr_set_code(pdu->hdr, COAP_CODE_INTERNAL_SERVER_ERROR);
    return coap_get_total_hdr_len(pdu);
}

static ssize_t coapfileserver_directory_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                                struct requestdata *request,
                                                struct coapfileserver_entry *resource)
{
    /**
     * ToDo:
     *
     * * Blockwise
     *
     */
    vfs_DIR dir;

    int err = vfs_opendir(&dir, request->namebuf);
    if (err != 0) {
        return coapfileserver_errno_handler(pdu, buf, len, err);
    }
    DEBUG("coapfileserver: Serving directory listing\n");

    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_LINK);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    vfs_dirent_t entry;
    ssize_t payload_cursor = 0;
    size_t root_len = resource->root ? strlen(resource->root) : 0;
    while (vfs_readdir(&dir, &entry) > 0) {
        char *entry_name = entry.d_name;
        size_t entry_len = strlen(entry_name);
        if (entry_len <= 2 && memcmp(entry_name, "..", entry_len) == 0) {
            /* Up pointers don't work the same way in URI semantics */
            continue;
        }
        bool is_dir = entry_is_dir(request->namebuf, entry_name);
        /* maybe ",", "<>", and the length */
        ssize_t need_bytes = (payload_cursor == 0 ? 0 : 1) + 2 + entry_len + (is_dir ? 1 : 0);
        if (payload_cursor + need_bytes > pdu->payload_len) {
            /* Without blockwise, this is the best approximation we can do */
            DEBUG("coapfileserver: Directory listing truncated\n");
            break;
        }
        if (payload_cursor != 0) {
            pdu->payload[payload_cursor++] = ',';
        }
        pdu->payload[payload_cursor++] = '<';

        payload_cursor += _copy_string(&pdu->payload[payload_cursor], resource->resource,
                                       pdu->payload_len - payload_cursor);
        payload_cursor += _copy_string(&pdu->payload[payload_cursor], &request->namebuf[root_len],
                                       pdu->payload_len - payload_cursor);
        pdu->payload[payload_cursor++] = '/';
        payload_cursor += _copy_string(&pdu->payload[payload_cursor], entry_name,
                                       pdu->payload_len - payload_cursor);
        if (is_dir) {
            pdu->payload[payload_cursor++] = '/';
        }
        pdu->payload[payload_cursor++] = '>';
    }
    vfs_closedir(&dir);

    if (payload_cursor == 0) {
        /* Rewind to clear payload marker */
        payload_cursor -= 1;
    }

    return resp_len + payload_cursor;
}

static size_t coapfileserver_errno_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, int err)
{
    uint8_t code;
    switch (err) {
        case -EACCES:
            code = COAP_CODE_FORBIDDEN;
            break;
        case -ENOENT:
            code = COAP_CODE_PATH_NOT_FOUND;
            break;
        default:
            code = COAP_CODE_INTERNAL_SERVER_ERROR;
    };
    DEBUG("coapfileserver: Rejecting error %d (%s) as %d.%02d\n", err, strerror(err),
          code >> 5, code & 0x1f);
    return gcoap_response(pdu, buf, len, code);
}

/** @} */
