/*
 * Copyright (C) 2023 ML!PA Consulting Gmbh
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 *
 * @{
 *
 * @file
 * @author  Fabian Hüßler <fabian.huessler@ml-pa.com>
 *
 * @}
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "configuration.h"
#include "container.h"
#include "embUnit.h"

#include "configuration_backend_ram.h"
#include "configuration_backend_flashdb.h"
#include "persist_types.h"

#define ENABLE_DEBUG    0
#include "debug.h"

#if defined(TEST_CONFIGURATION_BACKEND_RAM)
extern struct configuration persist_conf;
#else
static struct configuration persist_conf; /* dummy */
#endif

static char key_buf[32];

static const char *food_bread_keys[CONF_FOOD_BREAD_NUMOF] = {
    "white",
    "whole grain",
};

static const char *food_cake_keys[CONF_FOOD_CAKE_NUMOF] = {
    "cheesecake",
    "donut",
};

static const char *drinks_keys[CONF_DRINKS_NUMOF] = {
    "coffee",
    "tea",
    "cocoa",
};

struct configuration _conf = {
    .food = {
        .food_bread = {
            {"2.00"},
            {"2.20"}
        },
        .food_cake = {
            {"2.99"},
            {"2.00"},
        },
    },
    .drinks = {
        {"1.00"},
        {"1.00"},
        {"1.50"},
    }
};

static conf_handler_node_t _products_food_conf_handler
    = CONF_HANDLER_NODE_INITIALIZER("food");

static const conf_backend_t *_backend;

static int _any_food_set(const conf_handler_t *handler,
                         char key[], char *next, const void *val, size_t *size,
                         const char *keys[], food_t food[], unsigned food_numof)
{
    assert(handler);
    assert((val && size && *size) || (!val && !size));
    assert(key);
    assert(*key);
    assert(next);

    size_t sz = size ? *size : 0;
    int at = -1;
    if (!*next) {
        if (val) {
            if (sz < sizeof(food_t) * food_numof) {
                return -ENOBUFS;
            }
        }
        sz = sizeof(food_t) * food_numof;
        at = 0;
    }
    else {
        if(val) {
            if (sz < sizeof(food_t)) {
                return -ENOBUFS;
            }
        }
        sz = sizeof(food_t);
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
    }
    if (val) {
        *size -= sz;
        memcpy(&food[at], val, sz);
    }
    else {
        memset(&food[at], 0, sz);
    }
    return 0;
}

static int _any_food_get(const conf_handler_t *handler,
                         char key[], char *next, void *val, size_t *size,
                         const char *keys[], const food_t food[], unsigned food_numof)
{
    assert(handler);
    assert(val);
    assert(size);
    assert(key);
    assert(*key);
    assert(next);

    size_t sz;
    int at = -1;
    if (!*next) {
        sz = sizeof(food_t) * food_numof;
        at = 0;
    }
    else {
        sz = sizeof(food_t);
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1 || !*(food[at].price)) {
            return -ENOENT;
        }
    }
    if (*size < sz) {
        return -ENOBUFS;
    }
    *size -= sz;
    memcpy(val, &food[at], sz);
    return 0;
}

int _any_food_import(const conf_handler_t *handler,
                     char key[], char *next,
                     const char *keys[], food_t food[], unsigned food_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    if (!_backend) {
        return -ENODATA;
    }
    assert(_backend->ops);
    assert(_backend->ops->be_load);
    size_t sz = sizeof(food_t);
    int err;
    int at = -1;
    int key_len = strlen(key);
    if (!*next) {
        for (at = 0; (unsigned)at < food_numof; at++) {
            key[key_len] = '/';
            strcpy(&key[key_len + 1], keys[at]);
            if ((err = _backend->ops->be_load(_backend,
                                              key, &food[at], &sz))) {
                DEBUG("test configuration: backend import key %s failed (%d)\n", key, err);
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        key[key_len] = '/';
        strcpy(&key[key_len + 1], next);
        if ((err = _backend->ops->be_load(_backend,
                                          key, &food[at], &sz))) {
            DEBUG("test configuration: backend import key %s failed (%d)\n", key, err);
            return err;
        }
    }
    return 0;
}

int _any_food_export(const conf_handler_t *handler,
                     char key[], char *next,
                     const char *keys[], const food_t food[], unsigned food_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    if (!_backend) {
        return -ENODATA;
    }
    assert(_backend->ops);
    assert(_backend->ops->be_store);
    size_t sz = sizeof(food_t);
    int err;
    int at = -1;
    int key_len = strlen(key);
    if (!*next) {
        for (at = 0; (unsigned)at < food_numof; at++) {
            key[key_len] = '/';
            strcpy(&key[key_len + 1], keys[at]);
            if ((err = _backend->ops->be_store(_backend,
                                               key, &food[at], &sz))) {
                DEBUG("test configuration: backend export key %s failed (%d)\n", key, err);
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        key[key_len] = '/';
        strcpy(&key[key_len + 1], next);
        if ((err = _backend->ops->be_store(_backend,
                                           key, &food[at], &sz))) {
            DEBUG("test configuration: backend export key %s failed (%d)\n", key, err);
            return err;
        }
    }
    return 0;
}

int _any_food_delete(const conf_handler_t *handler,
                     char key[], char *next,
                     const char *keys[], unsigned food_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    if (!_backend) {
        return -ENODATA;
    }
    assert(_backend->ops);
    if (!_backend->ops->be_delete) {
        return -ENOTSUP;
    }
    int err;
    int at = -1;
    int key_len = strlen(key);
    if (!*next) {
        for (at = 0; (unsigned)at < food_numof; at++) {
            key[key_len] = '/';
            strcpy(&key[key_len + 1], keys[at]);
            if ((err = _backend->ops->be_delete(_backend, key))) {
                DEBUG("test configuration: backend delete key %s failed (%d)\n", key, err);
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        key[key_len] = '/';
        strcpy(&key[key_len + 1], next);
        if ((err = _backend->ops->be_delete(_backend, key))) {
            DEBUG("test configuration: backend delete key %s failed (%d)\n", key, err);
            return err;
        }
    }
    return 0;
}

static inline bool _verify_price(const char *price)
{
    const char *start = price;
    char *end = NULL;
    long num = strtoll(start, &end, 10);
    if (start == end || (errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))) {
        return false;
    }
    if (*end && *end != '.') {
        return false;
    }
    if (*end) {
        start = end + 1;
        end = NULL;
        num = strtoll(start, &end, 10);
        if (start == end || (errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))) {
            return false;
        }
    }
    return true;
}

int _any_food_verify(const struct conf_handler *handler,
                     char key[], char *next,
                     const char *keys[], const food_t food[], unsigned food_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    int at = -1;
    if (!*next) {
        for (at = 0; (unsigned)at < food_numof; at++) {
            if (!_verify_price(food[at].price)) {
                return -EINVAL;
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        if (!_verify_price(food[at].price)) {
            return -EINVAL;
        }
    }

    return 0;
}

int _any_food_apply(const struct conf_handler *handler,
                    char key[], char *next,
                    const char *keys[], const food_t food[], unsigned food_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    int at = -1;
    if (!*next) {
        for (at = 0; (unsigned)at < food_numof; at++) {
            DEBUG("test configuration: Applying %s to %s\n", food[at].price, keys[at]);
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < food_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        DEBUG("test configuration: Applying %s to %s\n", food[at].price, keys[at]);
    }

    return 0;
}

static inline int _food_bread_set(const conf_handler_t *handler,
                                  char key[], char *next, const void *val,
                                  size_t *size)
{
    return _any_food_set(handler, key, next, val, size, food_bread_keys,
                         _conf.food.food_bread, ARRAY_SIZE(_conf.food.food_bread));
}

static inline int _food_bread_get(const conf_handler_t *handler,
                                  char key[], char *next, void *val,
                                  size_t *size)
{
    return _any_food_get(handler, key, next, val, size, food_bread_keys,
                         _conf.food.food_bread, ARRAY_SIZE(_conf.food.food_bread));
}

static inline int _food_bread_import(const conf_handler_t *handler,
                                     char key[], char *next)
{
    return _any_food_import(handler, key, next, food_bread_keys,
                            _conf.food.food_bread, ARRAY_SIZE(_conf.food.food_bread));
}

static inline int _food_bread_export(const conf_handler_t *handler,
                                     char key[], char *next)
{
    return _any_food_export(handler, key, next, food_bread_keys,
                            _conf.food.food_bread, ARRAY_SIZE(_conf.food.food_bread));
}

static inline int _food_bread_delete(const conf_handler_t *handler,
                                     char key[], char *next)
{
    return _any_food_delete(handler, key, next, food_bread_keys,
                            ARRAY_SIZE(_conf.food.food_bread));
}

static inline int _food_bread_verify(const conf_handler_t *handler,
                                     char key[], char *next)
{
    return _any_food_verify(handler, key, next, food_bread_keys,
                            _conf.food.food_bread, ARRAY_SIZE(_conf.food.food_bread));
}

static inline int _food_bread_apply(const conf_handler_t *handler,
                                    char key[], char *next)
{
    return _any_food_apply(handler, key, next, food_bread_keys,
                           _conf.food.food_bread, ARRAY_SIZE(_conf.food.food_bread));
}


static const conf_handler_ops_t _food_bread_handler_ops = {
    .set = _food_bread_set,
    .get = _food_bread_get,
    .import = _food_bread_import,
    .export = _food_bread_export,
    .delete = _food_bread_delete,
    .verify = _food_bread_verify,
    .apply = _food_bread_apply,
};

static inline int _food_cake_set(const conf_handler_t *handler,
                                 char key[], char *next, const void *val,
                                 size_t *size)
{
    return _any_food_set(handler, key, next, val, size, food_cake_keys,
                         _conf.food.food_cake, ARRAY_SIZE(_conf.food.food_cake));
}

static inline int _food_cake_get(const conf_handler_t *handler,
                                 char key[], char *next, void *val,
                                 size_t *size)
{
    return _any_food_get(handler, key, next, val, size, food_cake_keys,
                         _conf.food.food_cake, ARRAY_SIZE(_conf.food.food_cake));
}

static inline int _food_cake_import(const conf_handler_t *handler,
                                    char key[], char *next)
{
    return _any_food_import(handler, key, next, food_cake_keys,
                            _conf.food.food_cake, ARRAY_SIZE(_conf.food.food_cake));
}

static inline int _food_cake_export(const conf_handler_t *handler,
                                    char key[], char *next)
{
    return _any_food_export(handler, key, next, food_cake_keys,
                            _conf.food.food_cake, ARRAY_SIZE(_conf.food.food_cake));
}

static inline int _food_cake_delete(const conf_handler_t *handler,
                                    char key[], char *next)
{
    return _any_food_delete(handler, key, next, food_cake_keys,
                            ARRAY_SIZE(_conf.food.food_cake));
}

static inline int _food_cake_verify(const conf_handler_t *handler,
                                    char key[], char *next)
{
    return _any_food_verify(handler, key, next, food_cake_keys,
                            _conf.food.food_cake, ARRAY_SIZE(_conf.food.food_cake));
}

static inline int _food_cake_apply(const conf_handler_t *handler,
                                    char key[], char *next)
{
    return _any_food_apply(handler, key, next, food_cake_keys,
                           _conf.food.food_cake, ARRAY_SIZE(_conf.food.food_cake));
}

static const conf_handler_ops_t _food_cake_handler_ops = {
    .set = _food_cake_set,
    .get = _food_cake_get,
    .import = _food_cake_import,
    .export = _food_cake_export,
    .delete = _food_cake_delete,
    .verify = _food_cake_verify,
    .apply = _food_cake_apply,
};

static int _any_drinks_set(const conf_handler_t *handler,
                           char key[], char *next, const void *val, size_t *size,
                           const char *keys[], drink_t drinks[], unsigned drinks_numof)
{
    assert(handler);
    assert((val && size && *size) || (!val && !size));
    assert(key);
    assert(*key);
    assert(next);

    size_t sz = size ? *size : 0;
    int at = -1;
    if (!*next) {
        if (val) {
            if (sz < sizeof(drink_t) * drinks_numof) {
                return -ENOBUFS;
            }
        }
        sz = sizeof(drink_t) * drinks_numof;
        at = 0;
    }
    else {
        if (val) {
            if (sz < sizeof(drink_t)) {
                return -ENOBUFS;
            }
        }
        sz = sizeof(drink_t);
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
    }
    if (val) {
        *size -= sz;
        memcpy(&drinks[at], val, sz);
    }
    else {
        memset(&drinks[at], 0, sz);
    }
    return 0;
}

static int _any_drinks_get(const conf_handler_t *handler,
                           char key[], char *next, void *val, size_t *size,
                           const char *keys[], const drink_t drinks[], unsigned drinks_numof)
{
    assert(handler);
    assert(val);
    assert(size);
    assert(key);
    assert(*key);
    assert(next);

    size_t sz;
    int at = -1;
    if (!*next) {
        sz = sizeof(drink_t) * drinks_numof;
        at = 0;
    }
    else {
        sz = sizeof(drink_t);
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1 || !*(drinks[at].price)) {
            return -ENOENT;
        }
    }
    if (*size < sz) {
        return -ENOBUFS;
    }
    *size -= sz;
    memcpy(val, &drinks[at], sz);
    return 0;
}

int _any_drinks_import(const conf_handler_t *handler,
                       char key[], char *next,
                       const char *keys[], drink_t drinks[], unsigned drinks_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    if (!_backend) {
        return -ENODATA;
    }
    assert(_backend->ops);
    assert(_backend->ops->be_load);
    size_t sz = sizeof(drink_t);
    int err;
    int at = -1;
    int key_len = strlen(key);
    if (!*next) {
        for (at = 0; (unsigned)at < drinks_numof; at++) {
            key[key_len] = '/';
            strcpy(&key[key_len + 1], keys[at]);
            if ((err = _backend->ops->be_load(_backend,
                                              key, &drinks[at], &sz))) {
                DEBUG("test configuration: backend import key %s failed (%d)\n", key, err);
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        key[key_len] = '/';
        strcpy(&key[key_len + 1], next);
        if ((err = _backend->ops->be_load(_backend,
                                          key, &drinks[at], &sz))) {
            DEBUG("test configuration: backend import key %s failed (%d)\n", key, err);
            return err;
        }
    }
    return 0;
}

int _any_drinks_export(const conf_handler_t *handler,
                       char key[], char *next,
                       const char *keys[], const drink_t drinks[], unsigned drinks_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    if (!_backend) {
        return -ENODATA;
    }
    assert(_backend->ops);
    assert(_backend->ops->be_store);
    size_t sz = sizeof(drink_t);
    int err;
    int at = -1;
    int key_len = strlen(key);
    if (!*next) {
        for(at = 0; (unsigned)at < drinks_numof; at++) {
            key[key_len] = '/';
            strcpy(&key[key_len + 1], keys[at]);
            if ((err = _backend->ops->be_store(_backend,
                                               key, &drinks[at], &sz))) {
                DEBUG("test configuration: backend export key %s failed (%d)\n", key, err);
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        key[key_len] = '/';
        strcpy(&key[key_len + 1], next);
        if ((err = _backend->ops->be_store(_backend,
                                           key, &drinks[at], &sz))) {
            DEBUG("test configuration: backend export key %s failed (%d)\n", key, err);
            return err;
        }
    }
    return 0;
}

int _any_drinks_delete(const conf_handler_t *handler,
                       char key[], char *next,
                       const char *keys[], unsigned drinks_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    if (!_backend) {
        return -ENODATA;
    }
    assert(_backend->ops);
    if (!_backend->ops->be_delete) {
        return -ENOTSUP;
    }
    int err;
    int at = -1;
    int key_len = strlen(key);
    if (!*next) {
        for(at = 0; (unsigned)at < drinks_numof; at++) {
            key[key_len] = '/';
            strcpy(&key[key_len + 1], keys[at]);
            if ((err = _backend->ops->be_delete(_backend, key))) {
                DEBUG("test configuration: backend delete key %s failed (%d)\n", key, err);
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        key[key_len] = '/';
        strcpy(&key[key_len + 1], next);
        if ((err = _backend->ops->be_delete(_backend, key))) {
            DEBUG("test configuration: backend delete key %s failed (%d)\n", key, err);
            return err;
        }
    }
    return 0;
}

int _any_drinks_verify(const struct conf_handler *handler,
                       char key[], char *next,
                       const char *keys[], const drink_t drinks[], unsigned drinks_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    int at = -1;
    if (!*next) {
        for (at = 0; (unsigned)at < drinks_numof; at++) {
            if (!_verify_price(drinks[at].price)) {
                return -EINVAL;
            }
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        if (!_verify_price(drinks[at].price)) {
            return -EINVAL;
        }
    }

    return 0;
}

int _any_drinks_apply(const struct conf_handler *handler,
                    char key[], char *next,
                    const char *keys[], const drink_t drinks[], unsigned drinks_numof)
{
    assert(handler);
    assert(key);
    assert(*key);
    assert(next);

    int at = -1;
    if (!*next) {
        for (at = 0; (unsigned)at < drinks_numof; at++) {
            DEBUG("test configuration: Applying %s to %s\n", drinks[at].price, keys[at]);
        }
    }
    else {
        for (unsigned i = 0; at == -1 && i < drinks_numof; i++) {
            if (!strcmp(keys[i], next)) {
                at = i;
            }
        }
        if (at == -1) {
            return -ENOENT;
        }
        DEBUG("test configuration: Applying %s to %s\n", drinks[at].price, keys[at]);
    }

    return 0;
}

static inline int _drinks_set(const conf_handler_t *handler,
                              char key[], char *next, const void *val,
                              size_t *size)
{
    return _any_drinks_set(handler, key, next, val, size, drinks_keys,
                           _conf.drinks, ARRAY_SIZE(_conf.drinks));
}

static inline int _drinks_get(const conf_handler_t *handler,
                              char key[], char *next, void *val,
                              size_t *size)
{
    return _any_drinks_get(handler, key, next, val, size, food_cake_keys,
                           _conf.drinks, ARRAY_SIZE(_conf.drinks));
}

static inline int _drinks_import(const conf_handler_t *handler,
                                 char key[], char *next)
{
    return _any_drinks_import(handler, key, next, drinks_keys,
                              _conf.drinks, ARRAY_SIZE(_conf.drinks));
}

static inline int _drinks_export(const conf_handler_t *handler,
                                 char key[], char *next)
{
    return _any_drinks_export(handler, key, next, drinks_keys,
                              _conf.drinks, ARRAY_SIZE(_conf.drinks));
}

static inline int _drinks_delete(const conf_handler_t *handler,
                                 char key[], char *next)
{
    return _any_drinks_delete(handler, key, next, drinks_keys,
                              ARRAY_SIZE(_conf.drinks));
}

static inline int _drinks_verify(const conf_handler_t *handler,
                                 char key[], char *next)
{
    return _any_drinks_verify(handler, key, next, drinks_keys,
                              _conf.drinks, ARRAY_SIZE(_conf.drinks));
}

static inline int _drinks_apply(const conf_handler_t *handler,
                                char key[], char *next)
{
    return _any_drinks_apply(handler, key, next, drinks_keys,
                             _conf.drinks, ARRAY_SIZE(_conf.drinks));
}

static const conf_handler_ops_t _drinks_handler_ops = {
    .set = _drinks_set,
    .get = _drinks_get,
    .import = _drinks_import,
    .export = _drinks_export,
    .delete = _drinks_delete,
    .verify = _drinks_verify,
    .apply = _drinks_apply,
};

static conf_handler_t _products_food_bread_handler
    = CONF_HANDLER_INITIALIZER("bread", &_food_bread_handler_ops);

static conf_handler_t _products_food_cake_handler
    = CONF_HANDLER_INITIALIZER("cake", &_food_cake_handler_ops);

static conf_handler_t _products_drinks_conf_handler
    = CONF_HANDLER_INITIALIZER("drinks", &_drinks_handler_ops);

static void _init_backend(void)
{
#if defined(TEST_CONFIGURATION_BACKEND_RAM)
    _backend = configuration_backend_ram_get();
#elif defined(TEST_CONFIGURATION_BACKEND_FLASHDB_MTD) || \
      defined(TEST_CONFIGURATION_BACKEND_FLASHDB_VFS)
    _backend = configuration_backend_flashdb_get();
#endif
}

static void _setup(void)
{
    configuration_register(configuration_get_root(), &_products_food_conf_handler);
    configuration_register(configuration_get_root(), &_products_drinks_conf_handler.node);
    configuration_register(&_products_food_conf_handler, &_products_food_bread_handler.node);
    configuration_register(&_products_food_conf_handler, &_products_food_cake_handler.node);
}

static void test_configuration_get(void)
{
    struct configuration conf;
    size_t conf_size;

    conf_size = sizeof(conf.food);
    strcpy(key_buf, "invalid");
    TEST_ASSERT(configuration_get(key_buf, &conf.food, &conf_size));

    conf_size = sizeof(conf.food);
    strcpy(key_buf, "/food/bread/invalid");
    TEST_ASSERT(configuration_get(key_buf, &conf.food, &conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food);
    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food, &_conf.food, conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food.food_bread);
    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food.food_bread, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food.food_bread, &_conf.food.food_bread, conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food.food_bread[0]);
    strcpy(key_buf, "/food/bread/white");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food.food_bread[0], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food.food_bread[0], &_conf.food.food_bread[0], conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food.food_bread[1]);
    strcpy(key_buf, "/food/bread/whole grain");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food.food_bread[1], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food.food_bread[1], &_conf.food.food_bread[1], conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food.food_cake);
    strcpy(key_buf, "/food/cake");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food.food_cake, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food.food_cake, &_conf.food.food_cake, conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food.food_cake[0]);
    strcpy(key_buf, "/food/cake/cheesecake");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food.food_cake[0], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food.food_cake[0], &_conf.food.food_cake[0], conf_size));

    memset(&conf, 0, sizeof(conf));
    conf_size = sizeof(conf.food.food_cake[1]);
    strcpy(key_buf, "/food/cake/donut");
    TEST_ASSERT(!configuration_get(key_buf, &conf.food.food_cake[1], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.food.food_cake[1], &_conf.food.food_cake[1], conf_size));

    conf_size = sizeof(conf.drinks);
    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_get(key_buf, &conf.drinks, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&conf.drinks, &_conf.drinks, conf_size));
}

static void test_configuration_set(void)
{
    struct configuration new_conf = {
        .food = {
            .food_bread = {
                {"2.50"},
                {"2.70"}
            },
            .food_cake = {
                {"3.50"},
                {"2.50"},
            }
        },
        .drinks = {
            {"1.50"},
            {"1.50"},
            {"2.00"},
        }
    };
    size_t conf_size;

    struct configuration conf_backup = _conf;

    conf_size = sizeof(new_conf.food);
    strcpy(key_buf, "invalid");
    TEST_ASSERT(configuration_set(key_buf, &new_conf.food, &conf_size));

    conf_size = sizeof(new_conf.food);
    strcpy(key_buf, "/food/bread/invalid");
    TEST_ASSERT(configuration_set(key_buf, &new_conf.food, &conf_size));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food.food_bread[0]);
    strcpy(key_buf, "/food/bread/white");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_bread[0], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_bread[0], &_conf.food.food_bread[0], sizeof(_conf.food.food_bread[0])));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food.food_bread[1]);
    strcpy(key_buf, "/food/bread/whole grain");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_bread[1], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_bread[1], &_conf.food.food_bread[1], sizeof(_conf.food.food_bread[1])));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food.food_bread);
    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_bread, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_bread, &_conf.food.food_bread, sizeof(_conf.food.food_bread)));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food.food_cake[0]);
    strcpy(key_buf, "/food/cake/cheesecake");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_cake[0], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_cake[0], &_conf.food.food_cake[0], sizeof(_conf.food.food_cake[0])));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food.food_cake[1]);
    strcpy(key_buf, "/food/cake/donut");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_cake[1], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_cake[1], &_conf.food.food_cake[1], sizeof(_conf.food.food_cake[1])));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food.food_cake);
    strcpy(key_buf, "/food/cake");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_cake, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_cake, &_conf.food.food_cake, sizeof(_conf.food.food_cake)));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.food);
    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food, &_conf.food, sizeof(_conf.food)));

    memset(&_conf, 0, sizeof(_conf));
    conf_size = sizeof(new_conf.drinks);
    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.drinks, &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.drinks, &_conf.drinks, sizeof(_conf.drinks)));

    _conf = conf_backup;
}

MAYBE_UNUSED
static void test_configuration_import(void)
{
    struct configuration conf_backup = _conf;

    strcpy(key_buf, "invalid");
    TEST_ASSERT(configuration_import(key_buf));

    strcpy(key_buf, "/food/bread/invalid");
    TEST_ASSERT(configuration_import(key_buf));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food, &_conf.food, sizeof(_conf.food)));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_bread, &_conf.food.food_bread, sizeof(_conf.food.food_bread)));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food/bread/white");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_bread[0], &_conf.food.food_bread[0], sizeof(_conf.food.food_bread[0])));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food/bread/whole grain");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_bread[1], &_conf.food.food_bread[1], sizeof(_conf.food.food_bread[1])));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food/cake");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_cake, &_conf.food.food_cake, sizeof(_conf.food.food_cake)));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food/cake/cheesecake");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_cake[0], &_conf.food.food_cake[0], sizeof(_conf.food.food_cake[0])));

    memset(&_conf, 0, sizeof(_conf));
    strcpy(key_buf, "/food/cake/donut");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_cake[1], &_conf.food.food_cake[1], sizeof(_conf.food.food_cake[1])));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.drinks, &_conf.drinks, sizeof(_conf.drinks)));

    _conf = conf_backup;
}

MAYBE_UNUSED
static void test_configuration_export(void)
{
    struct configuration new_conf = {
        .food = {
            .food_bread = {
                {"2.50"},
                {"2.70"}
            },
            .food_cake = {
                {"3.50"},
                {"2.50"},
            }
        },
        .drinks = {
            {"1.50"},
            {"1.50"},
            {"2.00"},
        }
    };
    struct configuration conf_backup = _conf;
    struct configuration persist_conf_backup = persist_conf;

    _conf = new_conf;

    strcpy(key_buf, "invalid");
    TEST_ASSERT(configuration_export(key_buf));

    strcpy(key_buf, "/food/bread/invalid");
    TEST_ASSERT(!configuration_export(key_buf));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food/bread/white");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&persist_conf.food.food_bread[0], &_conf.food.food_bread[0], sizeof(_conf.food.food_bread[0])));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food/bread/whole grain");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.food.food_bread[1], &_conf.food.food_bread[1], sizeof(_conf.food.food_bread[1])));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.food.food_bread, &_conf.food.food_bread, sizeof(_conf.food.food_bread)));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food/cake/cheesecake");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.food.food_cake[0], &_conf.food.food_cake[0], sizeof(_conf.food.food_cake[0])));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food/cake/donut");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.food.food_cake[1], &_conf.food.food_cake[1], sizeof(_conf.food.food_cake[1])));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food/cake");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.food.food_cake, &_conf.food.food_cake, sizeof(_conf.food.food_cake)));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.food, &_conf.food, sizeof(_conf.food)));

    memset(&persist_conf, 0, sizeof(persist_conf));
    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_export(key_buf));
    TEST_ASSERT(!memcmp(&new_conf.drinks, &_conf.drinks, sizeof(_conf.drinks)));

    _conf = conf_backup;
    persist_conf = persist_conf_backup;
}

static void test_configuration_import_modify_export_import(void)
{
    struct configuration new_conf = {
        .food = {
            .food_bread = {
                {"2.50"},
                {"2.70"}
            },
            .food_cake = {
                {"3.50"},
                {"2.50"},
            }
        },
        .drinks = {
            {"1.50"},
            {"1.50"},
            {"2.00"},
        }
    };
    struct configuration conf_backup = _conf;
    struct configuration persist_conf_backup = persist_conf;
    size_t conf_size;

    memset(&_conf, 0, sizeof(_conf));

    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_import(key_buf));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_import(key_buf));

    conf_size = sizeof(new_conf.food);
    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food, &conf_size));
    TEST_ASSERT(conf_size == 0);

    conf_size = sizeof(new_conf.drinks);
    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.drinks, &conf_size));
    TEST_ASSERT(conf_size == 0);

    strcpy(key_buf, "/food");
    configuration_delete(key_buf);

    strcpy(key_buf, "/drinks");
    configuration_delete(key_buf);

    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_export(key_buf));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_export(key_buf));

    memset(&_conf, 0, sizeof(_conf));

    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&_conf.food, &new_conf.food, sizeof(_conf.food)));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_import(key_buf));
    TEST_ASSERT(!memcmp(&_conf.drinks, &new_conf.drinks, sizeof(new_conf.drinks)));

    _conf = conf_backup;
    persist_conf = persist_conf_backup;
}

static void test_configuration_delete(void)
{
    struct configuration new_conf = {
        .food = {
            .food_bread = {
                {"2.50"},
                {"2.70"}
            },
            .food_cake = {
                {"3.50"},
                {"2.50"},
            }
        },
        .drinks = {
            {"1.50"},
            {"1.50"},
            {"2.00"},
        }
    };
    struct configuration conf_backup = _conf;
    struct configuration persist_conf_backup = persist_conf;
    size_t conf_size;
    int ret;

    strcpy(key_buf, "invalid");
    TEST_ASSERT(configuration_delete(key_buf));

    strcpy(key_buf, "/food/bread/baguette");
    TEST_ASSERT(!(ret = configuration_delete(key_buf)));

    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!(ret = configuration_delete(key_buf)));

    conf_size = sizeof(new_conf.food);
    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food, &conf_size));
    TEST_ASSERT(conf_size == 0);

    conf_size = sizeof(new_conf.drinks);
    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.drinks, &conf_size));
    TEST_ASSERT(conf_size == 0);

    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_export(key_buf));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_export(key_buf));


    if (_backend->ops->be_delete) {
        strcpy(key_buf, "/food");
        TEST_ASSERT(!configuration_delete(key_buf));
        strcpy(key_buf, "/food");
        TEST_ASSERT(!configuration_import(key_buf));
        strcpy(key_buf, "/food/bread/");
        strcat(key_buf, food_bread_keys[0]);
        TEST_ASSERT(configuration_import(key_buf));

        strcpy(key_buf, "/drinks");
        TEST_ASSERT(!configuration_delete(key_buf));
        strcpy(key_buf, "/drinks/");
        TEST_ASSERT(!configuration_import(key_buf));
        strcpy(key_buf, "/drinks/");
        strcat(key_buf, drinks_keys[0]);
        TEST_ASSERT(configuration_import(key_buf));
    }

    _conf = conf_backup;
    persist_conf = persist_conf_backup;
}

static void test_configuration_verify_apply(void)
{
    struct configuration new_conf = {
        .food = {
            .food_bread = {
                {"sale"},
                {"2.70"}
            },
            .food_cake = {
                {"3.50"},
                {"2.50"},
            }
        },
        .drinks = {
            {"free"},
            {"1.50"},
            {"2.00"},
        }
    };
    size_t conf_size;

    struct configuration conf_backup = _conf;

    strcpy(key_buf, "/food");
    TEST_ASSERT(!configuration_export(key_buf));

    conf_size = sizeof(new_conf.food.food_bread[0]);
    strcpy(key_buf, "/food/bread/");
    strcat(key_buf, food_bread_keys[0]);
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.food.food_bread[0], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.food.food_bread[0], &_conf.food.food_bread[0], sizeof(_conf.food.food_bread[0])));

    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(configuration_verify(key_buf, false));

    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!configuration_verify(key_buf, true));

    strcpy(key_buf, "/food/bread");
    TEST_ASSERT(!configuration_apply(key_buf));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_export(key_buf));

    conf_size = sizeof(new_conf.drinks[0]);
    strcpy(key_buf, "/drinks/");
    strcat(key_buf, drinks_keys[0]);
    TEST_ASSERT(!configuration_set(key_buf, &new_conf.drinks[0], &conf_size));
    TEST_ASSERT(conf_size == 0);
    TEST_ASSERT(!memcmp(&new_conf.drinks[0], &_conf.drinks[0], sizeof(_conf.drinks[0])));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(configuration_verify(key_buf, false));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_verify(key_buf, true));

    strcpy(key_buf, "/drinks");
    TEST_ASSERT(!configuration_apply(key_buf));

    _conf = conf_backup;
}

Test* test_configuration(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_configuration_get),
        new_TestFixture(test_configuration_set),
#if defined(TEST_CONFIGURATION_BACKEND_RAM)
        new_TestFixture(test_configuration_import),
        new_TestFixture(test_configuration_export),
#endif
        new_TestFixture(test_configuration_import_modify_export_import),
        new_TestFixture(test_configuration_delete),
        new_TestFixture(test_configuration_verify_apply),
    };

    EMB_UNIT_TESTCALLER(tests_configuration, NULL, NULL, fixtures);
    return (Test *)&tests_configuration;
}


int main(void)
{
    _setup();
    _init_backend();

    TESTS_START();
    TESTS_RUN(test_configuration());
    TESTS_END();

    puts("Tests done");
}
