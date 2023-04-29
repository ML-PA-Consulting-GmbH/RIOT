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
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "modules.h"
#include "container.h"
#include "configuration.h"
#include "mutex.h"

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
    unsigned sp;
    /**
     * @brief   Stack of handlers for iteration
     */
    const conf_handler_node_t *stack[CONFIGURATION_DEPTH_MAX + 1];
} conf_iterator_t;

static conf_handler_node_t _conf_root_handler
    = CONF_HANDLER_NODE_INITIALIZER("/");

static char *_configuration_key_next(char *key, const char *subkey)
{
    assert(key);
    assert(subkey);
    int len = strlen(subkey);
    /* not a subkey or not root and subkey does not match a full segment */
    if (strncmp(key, subkey, len) || (*key != '/' && key[len] && key[len] != '/')) {
        return NULL;
    }
    key += len;
    while (*key == '/') {
        key++;
    }
    return key;
}

static const conf_handler_node_t *_configuration_handler_next(const conf_handler_node_t *handler,
                                                              char *key)
{
    const conf_handler_node_t *sub = handler->sub_nodes;
    while (sub) {
        if (_configuration_key_next(key, sub->subtree)) {
            return sub;
        }
        sub = container_of(sub->node.next, conf_handler_node_t, node);
    }
    return NULL;
}

static int _configuration_find_handler(const conf_handler_node_t **next_handler, char **key)
{
    char *next = *key;
    const conf_handler_node_t *handler = *next_handler;
    if (!next || !handler ||
        !_configuration_key_next(*key, (*next_handler)->subtree)) {
        return -ENOENT;
    }
    while ((next = _configuration_key_next(*key, (*next_handler)->subtree))) {
        *key = next;
        if (!*next) {
            break;
        }
        if (!(handler = _configuration_handler_next(handler, next))) {
            break;
        }
        *next_handler = handler;
    }
    return 0;
}

static void _configuration_iterator_init(conf_iterator_t *iter,
                                         const conf_handler_node_t *handler)
{
    assert(iter);
    assert(handler);
    iter->root = handler;
    iter->sp = 0;
    iter->stack[iter->sp++] = handler;
}

static void _configuration_append_next(char key[], const conf_handler_node_t *next)
{
    /* need to find l '/' for a node on level l in the tree */
    char *slash = key;
    assert(*slash == '/');
    for (unsigned l = 0; l < next->level; l++, slash++) {
        slash = strchr(slash, '/');
        assert(slash);
    }
    strcpy(slash, next->subtree);
    strcat(key, "/");
}

static void _configuration_remove_trailing(char key[])
{
    /*present key without trailing slash */
    if (key[strlen(key) - 1] == '/') {
        key[strlen(key) - 1] = '\0';
    }
}

static conf_handler_t *_configuration_iterator_next(conf_iterator_t *iter, char key[])
{
    while (iter->sp > 0) {
        const conf_handler_node_t *next = iter->stack[--iter->sp];
        if (next != iter->root) {
            _configuration_append_next(key, next);
            if (next->node.next) {
                assert(iter->sp < ARRAY_SIZE(iter->stack));
                iter->stack[iter->sp++] = container_of(next->node.next, conf_handler_node_t, node);
            }
        }
        const conf_handler_node_t *subhandler = next->sub_nodes;
        if (subhandler) {
            assert(iter->sp < ARRAY_SIZE(iter->stack));
            iter->stack[iter->sp++] = subhandler;
        }
        else {
            _configuration_remove_trailing(key);
            return (conf_handler_t *)next;
        }
    }
    return NULL;
}

static int _configuration_prepare(const conf_handler_node_t **next_handler, char key[], char **next)
{
    *next = key;
    if (_configuration_find_handler(next_handler, next) < 0) {
        return -ENOENT;
    }
    if (**next) {
        if (&(*next)[-1] == key) {
            return -ENOENT;
        }
        assert((*next)[-1] == '/');
        (*next)[-1] = '\0';
    }
    else {
        if (key[strlen(key) - 1] != '/') {
            strcat(key, "/");
        }
        /* if there is no dynamic next part, "next" and "key" should not overlap in memory */
        *next = "";
    }
    return 0;
}

static int _configuration_handler_set_internal(const conf_handler_node_t *root,
                                               char key[], const void *value,
                                               size_t *size)
{
    assert(root);
    assert(key);
    assert((value && size && *size) || (!value && !size));

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        size_t sz = size ? *size : 0;
        assert(handler->ops->set);
        if (handler->ops->set(handler, key, next, value, size)) {
            return -ECANCELED;
        }
        if (value) {
            value = (const uint8_t *)value + (sz - *size);
        }
    }
    return 0;
}

static int _configuration_handler_get_internal(const conf_handler_node_t *root,
                                               char key[], void *value, size_t *size)
{
    assert(root);
    assert(key);
    assert(value);
    assert(size);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        size_t sz = *size;
        assert(handler->ops->get);
        if (handler->ops->get(handler, key, next, value, size)) {
            return -ECANCELED;
        }
        value = (uint8_t *)value + (sz - *size);
    }
    return 0;
}

static int _configuration_handler_import_internal(const conf_handler_node_t *root,
                                                  char key[])
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        if (handler->ops->import) {
            if (handler->ops->import(handler, key, next)) {
                return -ECANCELED;
            }
        }
    }
    return 0;
}

static int _configuration_handler_export_internal(const conf_handler_node_t *root,
                                                  char key[])
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        if (handler->ops->export) {
            if (handler->ops->verify) {
                if (handler->ops->verify(handler, key, next)) {
                    continue;
                }
            }
            if (handler->ops->export(handler, key, next)) {
                return -ECANCELED;
            }
        }
    }
    return 0;
}

static int _configuration_handler_delete_internal(const conf_handler_node_t *root,
                                                  char key[])
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        if (handler->ops->delete) {
            if (handler->ops->delete(handler, key, next)) {
                continue;
            }
        }
    }
    return 0;
}

static int _configuration_handler_apply_internal(const conf_handler_node_t *root,
                                                 char key[])
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        /* If this fails, there would be an inconsistency between verify() and applied values, or
           an API misuse where verify() was not called before. */
        if (handler->ops->apply) {
            handler->ops->apply(handler, key, next);
        }
    }
    return 0;
}

static int _configuration_handler_lock(const conf_handler_node_t *root, char key[])
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        mutex_lock(&handler->mutex);
    }
    return 0;
}

static int _configuration_handler_unlock(const conf_handler_node_t *root, char key[])
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        mutex_unlock(&handler->mutex);
    }
    return 0;
}

static int _configuration_handler_verify_internal(const conf_handler_node_t *root, char key[],
                                                  bool try_reimport)
{
    assert(root);
    assert(key);

    char *next;
    if (_configuration_prepare(&root, key, &next) < 0) {
        return -ENOENT;
    }
    conf_iterator_t iter;
    _configuration_iterator_init(&iter, root);
    conf_handler_t *handler;
    while ((handler = _configuration_iterator_next(&iter, key))) {
        if (handler->ops->verify) {
            if (handler->ops->verify(handler, key, next)) {
                if (!try_reimport || !handler->ops->import) {
                    return -ECANCELED;
                }
                else if (handler->ops->import(handler, key, next)) {
                    return -ECANCELED;
                }
            }
        }
    }
    return 0;
}

conf_handler_node_t *configuration_get_root(void)
{
    return &_conf_root_handler;
}

void configuration_register(conf_handler_node_t *parent, conf_handler_node_t *node)
{
    assert(parent);
    assert(node);
    conf_handler_node_t **end = &parent->sub_nodes;
    while (*end) {
        end = (conf_handler_node_t **)&(*end)->node.next;
    }
    node->level = parent->level + 1;
    *end = node;
}

int configuration_lock(char key[])
{
    return _configuration_handler_lock(configuration_get_root(), key);
}

int configuration_unlock(char key[])
{
    return _configuration_handler_unlock(configuration_get_root(), key);
}

int configuration_set(char key[], const void *value, size_t *size)
{
    return _configuration_handler_set_internal(configuration_get_root(), key, value, size);
}

int configuration_verify(char key[], bool try_reimport)
{
    return _configuration_handler_verify_internal(configuration_get_root(), key, try_reimport);
}

int configuration_get(char key[], void *value, size_t *size)
{
    return _configuration_handler_get_internal(configuration_get_root(), key, value, size);
}

int configuration_import(char key[])
{
    return _configuration_handler_import_internal(configuration_get_root(), key);
}

int configuration_export(char key[])
{
    return _configuration_handler_export_internal(configuration_get_root(), key);
}

int configuration_delete(char key[])
{
    return _configuration_handler_delete_internal(configuration_get_root(), key);
}

int configuration_apply(char key[])
{
    return _configuration_handler_apply_internal(configuration_get_root(), key);
}
