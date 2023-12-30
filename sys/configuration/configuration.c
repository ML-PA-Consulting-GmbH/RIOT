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
 * @brief       Implementation of a runtime configuration module
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
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>

#include "fmt.h"
#include "modules.h"
#include "container.h"
#include "configuration.h"
#include "mutex.h"

#define ENABLE_DEBUG 0
#include "debug.h"

/**
 * @brief   Iterator item type to retrieve when iterating over all nodes in the configuration tree
 */
typedef struct conf_iterator_item {
    /**
     * @brief   Configuration node to retrieve
     */
    const conf_handler_node_t *node;
} conf_iterator_item_t;

/**
 * @brief   Iterator type to iterate over the configuration tree non-recursively
 */
typedef struct configuration_iterator {
    /**
     * @brief   Root node to start from
     */
    const conf_handler_node_t *root;
    /**
     * @brief   Stack pointer
     */
    unsigned short sp;
    /**
     * @brief  Whether to retrieve all subhandlers or just the first one
     */
    bool max_depth;
    /**
     * @brief   Stack of handlers for iteration
     */
    conf_iterator_item_t stack[CONFIGURATION_DEPTH_MAX + 1];
} conf_iterator_t;

/**
 * @brief   Iterator item type to retrieve when iterating over all configuration path items
 *          in the configuration tree, including arrays
 */
typedef struct conf_path_iterator_item {
    /**
     * @brief   Configuration node to retrieve
     */
    const conf_handler_node_t *node;
    /**
     * @brief   Index when the node is an array
     */
    uint32_t index;
} conf_path_iterator_item_t;

/**
 * @brief   Iterator type to iterate over the configuration tree non-recursively
 */
typedef struct configuration_path_iterator {
    /**
     * @brief   Root node to start from
     */
    const conf_handler_node_t *root;
    /**
     * @brief   Stack pointer
     */
    unsigned short sp;
    /**
     * @brief  Whether to retrieve all subhandlers or just the first one
     */
    bool max_depth;
    /**
     * @brief   Stack of handlers for iteration
     */
    conf_path_iterator_item_t stack[CONFIGURATION_DEPTH_MAX + 1];
} conf_path_iterator_t;

static CONF_HANDLER_NODE_ID(_conf_root_handler_node_id, 0, UINT64_MAX, "");

static CONF_HANDLER_NODE(_conf_root_handler,
                         &_conf_root_handler_node_id);

static void _configuration_iterator_init(conf_iterator_t *iter,
                                         const conf_handler_node_t *handler,
                                         bool max_depth)
{
    assert(iter);
    assert(handler);
    iter->root = handler;
    iter->sp = 0;
    iter->max_depth = max_depth;
    iter->stack[iter->sp++] = (conf_iterator_item_t) {
        handler
    };
}

static void _configuration_path_iterator_init(conf_path_iterator_t *iter,
                                              const conf_handler_node_t *handler,
                                              bool max_depth,
                                              const conf_sid_t *sid)
{
    assert(iter);
    assert(handler);
    iter->root = handler;
    iter->sp = 0;
    iter->max_depth = max_depth;
    iter->stack[iter->sp++] = (conf_path_iterator_item_t) {
        handler,
        *sid > handler->node_id->sid_lower
            ? (*sid - handler->array_id->sid_lower - 1) / handler->array_id->sid_stride
            : 0
    };
}

static bool _sid_in_node_range(const conf_handler_node_t *node, conf_sid_t sid)
{
    return node->node_id->sid_lower <= sid && sid <= node->node_id->sid_upper;
}

static bool _sid_in_array_range(const conf_array_handler_t *node, conf_sid_t sid)
{
    return node->handler.node.array_id->sid_lower <= sid && sid <= node->handler.node.array_id->sid_upper;
}

static bool _sid_in_range(const conf_handler_node_t *node, conf_sid_t sid)
{
    if (node->ops && container_of(node, conf_handler_t, node)->conf_flags.handles_array) {
        return _sid_in_array_range(container_of(node, conf_array_handler_t, handler.node), sid);
    }
    else if (!node->ops) {
        return _sid_in_node_range(node, sid);
    }
    return container_of(node, conf_handler_t, node)->node.handler_id->sid_lower == sid;
}

static bool _sid_in_array_bounds(const conf_array_handler_t *array, conf_sid_t sid)
{
    if (sid > array->handler.node.array_id->sid_lower) {
        uint32_t index = (sid - array->handler.node.array_id->sid_lower - 1) /
                         array->handler.node.array_id->sid_stride;
        if (index >= array->array_size) {
            return false;
        }
    }
    return true;
}

static int _configuration_append_segment(const conf_handler_node_t *next, char *buf, size_t size)
{
    (void)next; (void)buf; (void)size;
#if IS_USED(MODULE_CONFIGURATION_STRINGS)
    if (*next->node_id->subtree) {
        for (int l = 0; l < (int)next->level - 1; l++) {
            if (*buf++ != '/') {
                return -EINVAL;
            }
            char *slash;
            if ((slash = strchr(buf, '/'))) {
                if (isdigit((int)slash[1])) {
                    if ((slash = strchr(++slash, '/'))) {
                        buf = slash;
                    }
                }
            }
        }
        *buf = '\0';
        if (size - strlen(buf) < 1 + strlen(next->node_id->subtree) + 1) {
            return -ENOBUFS;
        }
        strcat(buf, "/");
        strcat(buf, next->node_id->subtree);
    }
#endif
    return 0;
}

static void _debug_print(conf_sid_t sid, unsigned offset, const char *str)
{
    (void)str; (void)offset; (void)sid;
#if ENABLE_DEBUG
    char ssid[17];
    ssid[fmt_u64_hex(ssid, sid)] = '\0';
    DEBUG("configuration: %16s %10u %s\n", ssid, offset, str ? str : "");
#endif
}

static int _configuration_append_index(uint32_t index, char *buf, size_t size)
{
    (void)index; (void)buf; (void)size;
#if IS_USED(MODULE_CONFIGURATION_STRINGS)
    char sindex[11];
    sindex[fmt_u32_dec(sindex, index)] = '\0';
    if (size - strlen(buf) < 1 + strlen(sindex) + 1) {
        return -ENOBUFS;
    }
    strcat(buf, "/");
    strcat(buf, sindex);
#endif
    return 0;
}

static int _configuration_find_handler_sid(const conf_handler_node_t **next_handler,
                                           conf_sid_t *sid, unsigned *offset,
                                           char *str, size_t len)
{
    assert(next_handler);
    assert(*next_handler);
    assert(sid);
    assert(offset);

    const conf_handler_node_t *handler = *next_handler;

    if (!handler || !_sid_in_range(handler, *sid)) {
        return -ENOENT;
    }
    while (handler) {
        const conf_handler_node_t *sub;
        for (sub = handler->subnodes;
             sub;
             sub = container_of(sub->node.next, conf_handler_node_t, node)) {

            if (!_sid_in_range(sub, *sid)) {
                continue;
            }
            if (str && len && _configuration_append_segment(sub, str, len)) {
                return -ENOBUFS;
            }
            if (*sid != sub->node_id->sid_lower) {
                if (sub->ops && container_of(sub, conf_handler_t, node)->conf_flags.handles_array) {
                    uint32_t index = (*sid - sub->array_id->sid_lower - 1) /
                                    sub->array_id->sid_stride;
                    if (index >= container_of(sub, conf_array_handler_t, handler.node)->array_size) {
                        return -ERANGE;
                    }
                    /* Set data offset */
                    *offset += index * container_of(sub, conf_handler_t, node)->size;
                    /* map to first array element (normalize) */
                    *sid -= (index * sub->array_id->sid_stride);

                    if (str && len && _configuration_append_index(index, str, len)) {
                        return -ENOBUFS;
                    }
                }
            }
            break;
        }
        if ((handler = sub)) {
            *next_handler = handler;
        }
    }
    if (*sid != (*next_handler)->node_id->sid_lower) {
        if ((*next_handler)->ops && container_of((*next_handler), conf_handler_t, node)->conf_flags.handles_array) {
            if (!_sid_in_array_bounds(container_of((*next_handler), conf_array_handler_t, handler.node), *sid)) {
                return -ERANGE;
            }
        }
        else {
            return -ENOENT;
        }
    }
    _debug_print(*sid, *offset, str);
    return 0;
}


static int _configuration_prepare_sid(const conf_handler_node_t **next_handler, conf_key_buf_t *key)
{
    key->offset = 0;
    key->sid_normal = key->sid;
    char *buf = configuration_key_buf(key);
    if (buf) {
        *buf = '\0';
    }
    int ret = _configuration_find_handler_sid(next_handler, &key->sid_normal, &key->offset, buf, key->buf_len);
    if (ret < 0) {
        (void)ret;
        DEBUG("configuration: no handler found %d\n", ret);
    }
    return ret;
}

static conf_handler_node_t *_configuration_handler_sid_iterator_next(conf_iterator_t *iter, conf_key_buf_t *key)
{
    if (iter->sp > 0) {
        conf_iterator_item_t next = iter->stack[--iter->sp];
        if (next.node != iter->root) {
            if (_configuration_append_segment(next.node, configuration_key_buf(key), key->buf_len)) {
                return NULL;
            }
            key->sid += next.node->node_id->sid_lower - key->sid_normal;
            key->sid_normal = next.node->node_id->sid_lower;
            if (next.node->node.next) {
                assert(iter->sp < ARRAY_SIZE(iter->stack));
                iter->stack[iter->sp++] = (conf_iterator_item_t) {
                    container_of(next.node->node.next, conf_handler_node_t, node)
                };
            }
        }
        const conf_handler_node_t *subnodes = next.node->subnodes;
        if (subnodes) {
            if (!next.node->ops || iter->max_depth) {
                assert(iter->sp < ARRAY_SIZE(iter->stack));
                iter->stack[iter->sp++] = (conf_iterator_item_t) {
                    subnodes
                };
            }
        }
        _debug_print(key->sid, key->offset, configuration_key_buf(key));
        return (conf_handler_node_t *)next.node;
    }
    return NULL;
}

static conf_handler_node_t *_configuration_path_sid_iterator_next(conf_path_iterator_t *iter,
                                                                  conf_key_buf_t *key,
                                                                  const conf_sid_t *sid_start)
{
    if (iter->sp > 0) {
        conf_path_iterator_item_t next = iter->stack[--iter->sp];
        if (next.node->node_id->sid_lower > key->sid_normal) {
            key->sid += next.node->node_id->sid_lower - key->sid_normal;
            key->sid_normal = next.node->node_id->sid_lower;
        }
        if (key->sid != *sid_start && _configuration_append_segment(next.node, configuration_key_buf(key), key->buf_len)) {
            return NULL;
        }
        if (next.node->ops && container_of(next.node, conf_handler_t, node)->conf_flags.handles_array
                           && container_of(next.node, conf_handler_t, node)->conf_flags.export_as_a_whole == 0) {

            bool skip = key->sid == *sid_start && next.node->node_id->sid_lower < key->sid_normal;
            if (next.index == 0) {
                if (key->sid_normal == next.node->node_id->sid_lower) {
                    key->sid++;
                    (key->sid_normal)++;
                }
            }
            else if (key->sid != *sid_start) {
                if (next.node->node_id->sid_lower + 1 + next.index * next.node->array_id->sid_stride > key->sid) {
                    key->sid = next.node->node_id->sid_lower + 1 + next.index * next.node->array_id->sid_stride;
                }
                else {
                    key->sid += (next.node->array_id->sid_stride - (key->sid_normal - (next.node->array_id->sid_lower + 1)));
                }
                key->sid_normal = next.node->node_id->sid_lower + 1;
                key->offset = next.index * container_of(next.node, conf_handler_t, node)->size;
            }
            if (key->sid != *sid_start && _configuration_append_index(next.index, configuration_key_buf(key), key->buf_len)) {
                return NULL;
            }
            if (!skip) {
                if (++next.index < container_of(next.node, conf_array_handler_t, handler.node)->array_size) {
                    assert(iter->sp < ARRAY_SIZE(iter->stack));
                    iter->stack[iter->sp++] = next;
                }
                else if (next.node != iter->root && next.node->node.next) {
                    assert(iter->sp < ARRAY_SIZE(iter->stack));
                    iter->stack[iter->sp++] = (conf_path_iterator_item_t) {
                        container_of(next.node->node.next, conf_handler_node_t, node), 0
                    };
                }
            }
        }
        else {
            key->sid_normal = next.node->node_id->sid_lower;
            if (next.node != iter->root && next.node->node.next) {
                assert(iter->sp < ARRAY_SIZE(iter->stack));
                iter->stack[iter->sp++] = (conf_path_iterator_item_t) {
                    container_of(next.node->node.next, conf_handler_node_t, node), 0
                };
            }
        }
        const conf_handler_node_t *subnodes = next.node->subnodes;
        if (subnodes) {
            bool array_as_a_whole = next.node->ops && container_of(next.node, conf_handler_t, node)->conf_flags.handles_array
                                                   && container_of(next.node, conf_handler_t, node)->conf_flags.export_as_a_whole;
            if (!next.node->ops || iter->max_depth) {
                if (!array_as_a_whole) {
                    assert(iter->sp < ARRAY_SIZE(iter->stack));
                    iter->stack[iter->sp++] = (conf_path_iterator_item_t) {
                        subnodes, 0
                    };
                }
            }
        }
        _debug_print(key->sid, key->offset, configuration_key_buf(key));
        return (conf_handler_node_t *)next.node;
    }
    return NULL;
}

static int _configuration_handler_set_internal(const conf_handler_node_t *root,
                                               conf_key_buf_t *key, const void *value,
                                               size_t *size)
{
    assert(root);
    assert(key);
    assert((value && size && *size) || (!value && !size));

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, false);

    int ret = 0;
    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops || !handler->ops->set) {
            continue;
        }
        size_t sz = size ? *size : 0;
        if ((ret = handler->ops->set(container_of(handler, conf_handler_t, node), key, value, size))) {
            break;
        }
        if (value) {
            value = (const uint8_t *)value + (sz - *size);
        }
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return ret;
}

static int _configuration_handler_get_internal(const conf_handler_node_t *root,
                                               conf_key_buf_t *key, void *value, size_t *size)
{
    assert(root);
    assert(key);
    assert(value);
    assert(size);

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, false);

    int ret = 0;
    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops || !handler->ops->get) {
            continue;
        }
        size_t sz = *size;
        if ((ret = handler->ops->get(container_of(handler, conf_handler_t, node), key, value, size))) {
            break;
        }
        value = (uint8_t *)value + (sz - *size);
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return ret;
}

static int _configuration_handler_import_internal(const conf_handler_node_t *root,
                                                  conf_key_buf_t *key)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_path_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_path_iterator_init(&iter, root, true, &key->sid);

    conf_handler_node_t *handler;
    while ((handler = _configuration_path_sid_iterator_next(&iter, key, &sid))) {
        if (!handler->ops || !handler->ops->import) {
            continue;
        }
        handler->ops->import(container_of(handler, conf_handler_t, node), key);
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return 0;
}

static int _configuration_handler_export_internal(const conf_handler_node_t *root,
                                                  conf_key_buf_t *key)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_path_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_path_iterator_init(&iter, root, true, &key->sid);

    int ret = 0;
    conf_handler_node_t *handler;
    while ((handler = _configuration_path_sid_iterator_next(&iter, key, &sid))) {
        if (!handler->ops || !handler->ops->export) {
            continue;
        }
        if (handler->ops_dat && handler->ops_dat->verify) {
            if (handler->ops_dat->verify(container_of(handler, conf_handler_t, node), key)) {
                continue;
            }
        }
        if ((ret = handler->ops->export(container_of(handler, conf_handler_t, node), key))) {
            break;
        }
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return ret;
}

static int _configuration_handler_delete_internal(const conf_handler_node_t *root,
                                                  conf_key_buf_t *key)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_path_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_path_iterator_init(&iter, root, true, &key->sid);

    conf_handler_node_t *handler;
    while ((handler = _configuration_path_sid_iterator_next(&iter, key, &sid))) {
        if (!handler->ops || !handler->ops->delete) {
            continue;
        }
        handler->ops->delete(container_of(handler, conf_handler_t, node), key);
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return 0;
}

static int _configuration_handler_apply_internal(const conf_handler_node_t *root,
                                                 conf_key_buf_t *key)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, false);

    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops_dat || !handler->ops_dat->apply) {
            continue;
        }
        /* If this fails, there would be an inconsistency between verify() and applied values, or
           an API misuse where verify() was not called before. */
        handler->ops_dat->apply(container_of(handler, conf_handler_t, node), key);
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return 0;
}

static int _configuration_handler_lock(const conf_handler_node_t *root, conf_key_buf_t *key)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, true);

    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops) {
            continue;
        }
        mutex_lock(&container_of(handler, conf_handler_t, node)->mutex);
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return 0;
}

static int _configuration_handler_unlock(const conf_handler_node_t *root, conf_key_buf_t *key)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, true);

    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops) {
            continue;
        }
        mutex_unlock(&container_of(handler, conf_handler_t, node)->mutex);
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return 0;
}

static int _configuration_handler_verify_internal(const conf_handler_node_t *root, conf_key_buf_t *key,
                                                  bool try_reimport)
{
    assert(root);
    assert(key);

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, false);

    int ret = 0;
    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops_dat || !handler->ops_dat->verify) {
            continue;
        }
        if (handler->ops_dat->verify(container_of(handler, conf_handler_t, node), key)) {
            if (!try_reimport || !handler->ops->import) {
                ret = -ECANCELED;
                break;
            }
            else if ((ret = _configuration_handler_import_internal(configuration_get_root(), key))) {
                break;
            }
            if (handler->ops_dat->verify(container_of(handler, conf_handler_t, node), key)) {
                ret = -ECANCELED;
                break;
            }
        }
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return ret;
}

static int _configuration_set_backend_internal(const conf_handler_node_t *root, conf_key_buf_t *key,
                                               const conf_backend_t *src_backend,
                                               const conf_backend_t *dst_backend)
{
    assert(root);
    assert(key);
    assert(src_backend);

    conf_sid_t sid = key->sid;
    conf_iterator_t iter;

    if (_configuration_prepare_sid(&root, key) < 0) {
        return -ENOENT;
    }
    int key_len = configuration_key_buf(key) ? strlen(configuration_key_buf(key)) : 0;
    _configuration_iterator_init(&iter, root, false);

    int ret = 0;
    conf_handler_node_t *handler;
    while ((handler = _configuration_handler_sid_iterator_next(&iter, key))) {
        if (!handler->ops) {
            continue;
        }
        container_of(handler, conf_handler_t, node)->src_backend = src_backend;
        container_of(handler, conf_handler_t, node)->dst_backend = dst_backend;
    }
    if (key_len) {
        configuration_key_buf(key)[key_len] = '\0';
    }
    key->sid = sid;
    return ret;
}

conf_handler_node_t *configuration_get_root(void)
{
    return &_conf_root_handler;
}

void configuration_register(conf_handler_node_t *parent, conf_handler_node_t *node)
{
    assert(parent);
    assert(node);
    assert(!node->node.next); /* must be registered from the root to the leafs */
    conf_handler_node_t **end = &parent->subnodes;
    while (*end) {
        end = (conf_handler_node_t **)&(*end)->node.next;
    }
    node->level = parent->level + 1;
    *end = node;
}

int configuration_lock(conf_key_t *key)
{
    return _configuration_handler_lock(configuration_get_root(), key);
}

int configuration_unlock(conf_key_t *key)
{
    return _configuration_handler_unlock(configuration_get_root(), key);
}

int configuration_set(conf_key_t *key, const void *value, size_t *size)
{
    return _configuration_handler_set_internal(configuration_get_root(), key, value, size);
}

int configuration_verify(conf_key_t *key, bool try_reimport)
{
    return _configuration_handler_verify_internal(configuration_get_root(), key, try_reimport);
}

int configuration_get(conf_key_t *key, void *value, size_t *size)
{
    return _configuration_handler_get_internal(configuration_get_root(), key, value, size);
}

int configuration_import(conf_key_t *key)
{
    return _configuration_handler_import_internal(configuration_get_root(), key);
}

int configuration_export(conf_key_t *key)
{
    return _configuration_handler_export_internal(configuration_get_root(), key);
}

int configuration_delete(conf_key_t *key)
{
    return _configuration_handler_delete_internal(configuration_get_root(), key);
}

int configuration_apply(conf_key_t *key)
{
    return _configuration_handler_apply_internal(configuration_get_root(), key);
}

int configuration_set_backend(conf_key_t *key,
                              const conf_backend_t *src_backend,
                              const conf_backend_t *dst_backend)
{
    return _configuration_set_backend_internal(configuration_get_root(), key,
                                               src_backend, dst_backend);
}
