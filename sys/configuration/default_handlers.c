/*
 * Copyright (C) 2023 ML!PA Consulting Gmbh
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     sys_configuration
 * @{
 *
 * @file
 * @brief       Implementation of runtime configuration default handlers
 *
 * These handler implementations can be used if no dynamic key part is used.
 * This is the case if every path segment in the key has a node in the configuration tree.
 *
 * @author      Fabian Hüßler <fabian.huessler@ml-pa.com>
 *
 * @}
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "fmt.h"
#include "modules.h"
#include "container.h"
#include "configuration.h"

#define ENABLE_DEBUG    0
#include "debug.h"

int configuration_set_handler_default(const conf_handler_t *handler,
                                      conf_key_buf_t *key, const void *val,
                                      size_t *size)
{
    assert(handler);
    assert(key);
    assert((val && size && *size) || (!val && !size));

    size_t sz = size ? *size : 0;
    if (handler->conf_flags.handles_array && handler->node.array_id->sid_lower == key->sid_normal) {
        if (val && sz < handler->size * container_of(handler, conf_array_handler_t, handler)->array_size) {
            return -ENOBUFS;
        }
        sz = handler->size * container_of(handler, conf_array_handler_t, handler)->array_size;
    }
    else {
        if (val && sz < handler->size) {
            return -ENOBUFS;
        }
        sz = handler->size;
    }
    uint8_t *data = handler->data;
    if (key->offset > 0) {
        data += key->offset;
    }
    if (val) {
        memcpy(data, val, sz);
        *size -= sz;
    }
    else {
        memset(data, 0, sz);
    }
    return 0;
}

int configuration_get_handler_default(const conf_handler_t *handler,
                                      conf_key_buf_t *key, void *val,
                                      size_t *size)
{
    assert(handler);
    assert(val);
    assert(key);
    assert(size);

    size_t sz = handler->size;
    const uint8_t *data = handler->data;
    if (key->offset > 0) {
        data += key->offset;
    }
    *size -= sz;
    memcpy(val, data, sz);
    return 0;
}

int configuration_import_handler_default(const conf_handler_t *handler,
                                         conf_key_buf_t *key)
{
    assert(handler);
    assert(key);

    if (!handler->src_backend) {
        return -ENODATA;
    }

    assert(handler->src_backend->ops);
    assert(handler->src_backend->ops->be_load);

    conf_sid_t sid_restore = key->sid;
    size_t sz = handler->size;
    if (handler->conf_flags.handles_array &&
            handler->node.array_id->sid_lower == key->sid_normal &&
                handler->conf_flags.export_as_a_whole) {
        /* the only case where the import size is not equal to handler->size */
        sz = handler->size * container_of(handler, conf_array_handler_t, handler)->array_size;
    }
    uint8_t *data = handler->data;
    if (key->offset > 0) {
        data += key->offset;
    }
    int err; (void)err;
    if (!handler->conf_flags.handles_array ||
            handler->node.array_id->sid_lower != key->sid_normal ||
                handler->conf_flags.export_as_a_whole) {
        if (handler->conf_flags.handles_array &&
                handler->node.array_id->sid_lower != key->sid_normal &&
                    handler->conf_flags.export_as_a_whole) {
            DEBUG("configuration: importing array items is not supported for key %s\n",
                  configuration_key_buf(key) ? configuration_key_buf(key) : "");
            goto restore_key;
        }
        void *data_cpy = data;
        void *dec_data = NULL;
        size_t dec_size;
        if (handler->node.ops_dat && handler->node.ops_dat->decode) {
            /* query decoding buffer */
            handler->node.ops_dat->decode(handler, key, &dec_data, &dec_size);
            assert(dec_data);
            assert(dec_size);
            data = dec_data; /* decode() has decoded the data and returns decoded representation */
            sz = dec_size; /* decode() has decoded the data and returns the decoded data size */
        }
        if ((err = handler->src_backend->ops->be_load(handler->src_backend, key, data, &sz))) {
            DEBUG("configuration: backend importing key %s failed (%d)\n",
                  configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
        }
        else if (handler->node.ops_dat && handler->node.ops_dat->decode) {
            dec_data = data;
            dec_size = dec_size - sz;
            if ((err = handler->node.ops_dat->decode(handler, key, &dec_data, &dec_size))) {
                DEBUG("configuration: decoding for key %s failed (%d)\n",
                      configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
            }
            else {
                /* The decoder should not directly decode to the data location because decoding could fail */
                memcpy(data_cpy, dec_data, dec_size);
            }
        }
    }
    else {
        /* handler->conf_flags.handles_array &&
                handler->node.array_id->sid_lower == key->sid_normal &&
                    !handler->conf_flags.export_as_a_whole */
        assert(handler->conf_flags.handles_array);
        assert(handler->node.array_id->sid_lower == key->sid_normal);
        const conf_array_handler_t *array = container_of(handler, conf_array_handler_t, handler);
        unsigned at;
        key->sid++;
        for (at = 0; at < array->array_size; at++, data += handler->size) {
            void *data_cpy = data;
            void *dec_data = NULL;
            size_t dec_size;
            if (handler->node.ops_dat && handler->node.ops_dat->decode) {
                /* query decoding buffer */
                handler->node.ops_dat->decode(handler, key, &dec_data, &dec_size);
                assert(dec_data);
                assert(dec_size);
                data = dec_data; /* decode() has decoded the data and returns decoded representation */
                sz = dec_size; /* decode() has decoded the data and returns the decoded data size */
            }
            if ((err = handler->src_backend->ops->be_load(handler->src_backend, key, data, &sz))) {
                DEBUG("configuration: backend importing key %s failed (%d)\n",
                      configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
            }
            else if (handler->node.ops_dat && handler->node.ops_dat->decode) {
                dec_data = data;
                dec_size = dec_size - sz;
                if ((err = handler->node.ops_dat->decode(handler, key, &dec_data, &dec_size))) {
                    DEBUG("configuration: decoding for key %s failed (%d)\n",
                          configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
                }
                else {
                    /* The decoder should not directly decode to the data location because decoding could fail */
                    memcpy(data_cpy, dec_data, dec_size);
                }
            }
            key->sid += handler->node.array_id->sid_stride;
            data = data_cpy;
            sz = handler->size;
        }
    }
restore_key:
    key->sid = sid_restore;
    return 0;
}

int configuration_export_handler_default(const conf_handler_t *handler,
                                         conf_key_buf_t *key)
{
    assert(handler);
    assert(key);

    if (!handler->src_backend) {
        return -ENODATA;
    }
    assert(handler->src_backend->ops);
    assert(handler->src_backend->ops->be_store);

    conf_sid_t sid_restore = key->sid;
    size_t sz = handler->size;
    if (handler->conf_flags.handles_array &&
            handler->node.array_id->sid_lower == key->sid_normal &&
                handler->conf_flags.export_as_a_whole) {
        /* the only case where the export size is not equal to handler->size */
        sz = handler->size * container_of(handler, conf_array_handler_t, handler)->array_size;
    }
    const uint8_t *data = handler->data;
    if (key->offset > 0) {
        data += key->offset;
    }
    int err; (void)err;
    if (!handler->conf_flags.handles_array ||
            handler->node.array_id->sid_lower != key->sid_normal ||
                handler->conf_flags.export_as_a_whole) {
        if (handler->conf_flags.handles_array &&
                handler->node.array_id->sid_lower != key->sid_normal &&
                    handler->conf_flags.export_as_a_whole) {
            DEBUG("configuration: exporting array items is not supported for key %s\n",
                  configuration_key_buf(key) ? configuration_key_buf(key) : "");
            goto restore_key;
        }
        if (handler->node.ops_dat && handler->node.ops_dat->encode) {
            const void *enc_data = data;
            size_t enc_size = sz;
            if ((err = handler->node.ops_dat->encode(handler, key, &enc_data, &enc_size))) {
                DEBUG("configuration: encoding for key %s failed (%d)\n",
                      configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
                goto restore_key;
            }
            data = enc_data; /* encode() has encoded the data and returns encoded representation */
            sz = enc_size; /* encode() has encoded the data and returns the encoded data size */
        }
        if ((err = handler->src_backend->ops->be_store(handler->src_backend, key, data, &sz))) {
            DEBUG("configuration: backend exporting key %s failed (%d)\n",
                  configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
        }
    }
    else {
        /* handler->conf_flags.handles_array &&
            handler->node.array_id->sid_lower == key->sid_normal &&
                !handler->conf_flags.export_as_a_whole */
        assert(handler->conf_flags.handles_array);
        assert(handler->node.array_id->sid_lower == key->sid_normal);
        const conf_array_handler_t *array = container_of(handler, conf_array_handler_t, handler);
        unsigned at;
        key->sid++;
        for (at = 0; at < array->array_size; at++, data += handler->size) {
            const void *data_cpy = data;
            if (handler->node.ops_dat && handler->node.ops_dat->encode) {
                const void *enc_data = data;
                size_t enc_size;
                if ((err = handler->node.ops_dat->encode(handler, key, &enc_data, &enc_size))) {
                    DEBUG("configuration: encoding for key %s failed (%d)\n",
                          configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
                    continue;
                }
                data = enc_data; /* encode() has encoded the data and returns encoded representation */
                sz = enc_size;
            }
            if ((err = handler->src_backend->ops->be_store(handler->src_backend, key, data, &sz))) {
                DEBUG("configuration: backend exporting key %s failed (%d)\n",
                      configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
            }
            key->sid += handler->node.array_id->sid_stride;
            data = data_cpy;
            sz = handler->size;
        }
    }
restore_key:
    key->sid = sid_restore;
    return 0;
}

int configuration_delete_handler_default(const conf_handler_t *handler,
                                         conf_key_buf_t *key)
{
    assert(handler);
    assert(key);

    if (!handler->src_backend) {
        return -ENODATA;
    }
    assert(handler->src_backend->ops);
    if (!handler->src_backend->ops->be_delete) {
        return -ENOTSUP;
    }
    int err; (void)err;
    conf_sid_t sid_restore = key->sid;
    if (!handler->conf_flags.handles_array ||
            handler->node.array_id->sid_lower != key->sid_normal ||
                handler->conf_flags.export_as_a_whole) {
        if (handler->conf_flags.handles_array &&
                handler->node.array_id->sid_lower != key->sid_normal &&
                    handler->conf_flags.export_as_a_whole) {
            DEBUG("configuration: deleting array items is not supported for key %s\n",
                  configuration_key_buf(key) ? configuration_key_buf(key) : "");
            goto restore_key;
        }
        if ((err = handler->src_backend->ops->be_delete(handler->src_backend, key))) {
            DEBUG("configuration: backend deleting key %s failed (%d)\n",
                  configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
        }
    }
    else {
        /* handler->conf_flags.handles_array &&
            handler->node.array_id->sid_lower == key->sid_normal &&
                !handler->conf_flags.export_as_a_whole */
        assert(handler->conf_flags.handles_array);
        assert(handler->node.array_id->sid_lower == key->sid_normal);
        const conf_array_handler_t *array = container_of(handler, conf_array_handler_t, handler);
        unsigned at;
        key->sid++;
        for (at = 0; at < array->array_size; at++) {
            if ((err = handler->src_backend->ops->be_delete(handler->src_backend, key))) {
                DEBUG("configuration: backend deleting key %s failed (%d)\n",
                      configuration_key_buf(key) ? configuration_key_buf(key) : "", err);
            }
            key->sid += handler->node.array_id->sid_stride;
        }
    }
restore_key:
    key->sid = sid_restore;
    return 0;
}

const conf_handler_ops_t configuration_default_ops = {
    .set = configuration_set_handler_default,
    .get = configuration_get_handler_default,
    .import = configuration_import_handler_default,
    .export = configuration_export_handler_default,
    .delete = configuration_delete_handler_default,
};
