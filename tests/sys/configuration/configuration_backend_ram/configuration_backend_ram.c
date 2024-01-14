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
 * @file    Implementation of test backend in RAM
 *
 * @author  Fabian Hüßler <fabian.huessler@ml-pa.com>
 *
 * @}
 */

#include <errno.h>
#include <string.h>

#include "configuration.h"

#include "container.h"
#include "persist_types.h"

struct configuration persist_conf = {
    .food = {
        .bread = {
            .white = {"1.00"},
            .whole_grain = {"1.20"},
        },
        .cake = {
            .cheesecake = {"1.99"},
            . donut = {"1.00"},
        },
    },
    .drinks = {
        .coffee = {"0.50"},
        .tea = {"0.60"},
        .cocoa = {"1.00"},
    },
    .orders = {
        { .items = { {"sugar"}, {"tomatoes"} } },
        { .items = { {"coffee"}, {"milk"} } },
        { .items = { {"bread"}, {"coffee"} } },
    },
};

struct kv {
    const char *key;
    conf_sid_t sid;
    void *value;
    bool deleted;
};

/* A real backend would not have to store the keys statically */
static struct kv _kv[] = {
    {"/food/bread/white", TEST_FOOD_BREAD_WHITE_SID, &persist_conf.food.bread.white, false},
    {"/food/bread/whole_grain", TEST_FOOD_BREAD_WHOLE_GRAIN_SID, &persist_conf.food.bread.whole_grain, false},

    {"/food/cake/cheesecake", TEST_FOOD_CAKE_CHEESECAKE_SID, &persist_conf.food.cake.cheesecake, false},
    {"/food/cake/donut", TEST_FOOD_CAKE_DONUT_SID, &persist_conf.food.cake.donut, false},

    {"/drinks/coffee", TEST_DRINKS_COFFEE_SID, &persist_conf.drinks.coffee, false},
    {"/drinks/tea", TEST_DRINKS_TEA_SID, &persist_conf.drinks.tea, false},
    {"/drinks/cocoa", TEST_DRINKS_COCOA_SID, &persist_conf.drinks.cocoa, false},

    {"/orders/0", TEST_ORDERS_LOWER_SID + TEST_ORDERS_INDEX_LOWER_SID + (0 * TEST_ORDERS_INDEX_STRIDE), &persist_conf.orders[0], false},
    {"/orders/1", TEST_ORDERS_LOWER_SID + TEST_ORDERS_INDEX_LOWER_SID + (1 * TEST_ORDERS_INDEX_STRIDE), &persist_conf.orders[1], false},
    {"/orders/2", TEST_ORDERS_LOWER_SID + TEST_ORDERS_INDEX_LOWER_SID + (2 * TEST_ORDERS_INDEX_STRIDE), &persist_conf.orders[2], false},
};

static int _be_ram_load(const struct conf_backend *be,
                        conf_key_buf_t *key, void *val, size_t *size)
{
    (void)be;
    for (unsigned i = 0; i < ARRAY_SIZE(_kv); i++) {
        if ((!strcmp(configuration_key_buf(key) ? configuration_key_buf(key) : "", _kv[i].key) ||
            _kv[i].sid == key->sid) && !_kv[i].deleted) {
            memcpy(val, _kv[i].value, *size);
            return 0;
        }
    }
    return -ENOENT;
}

static int _be_ram_store(const struct conf_backend *be,
                         conf_key_buf_t *key, const void *val, size_t *size)
{
    (void)be;
    for (unsigned i = 0; i < ARRAY_SIZE(_kv); i++) {
        if (!strcmp(configuration_key_buf(key) ? configuration_key_buf(key) : "", _kv[i].key) ||
            _kv[i].sid == key->sid) {
            memcpy(_kv[i].value, val, *size);
            _kv[i].deleted = false;
            return 0;
        }
    }
    return -ENOENT;
}

static int _be_ram_delete(const struct conf_backend *be,
                          conf_key_buf_t *key)
{
    (void)be;
    for (unsigned i = 0; i < ARRAY_SIZE(_kv); i++) {
        if (!strcmp(configuration_key_buf(key) ? configuration_key_buf(key) : "", _kv[i].key) ||
            _kv[i].sid == key->sid) {
            _kv[i].deleted = true;
            return 0;
        }
    }
    return -ENOENT;
}

static const conf_backend_ops_t _be_ram_ops = {
    .be_load = _be_ram_load,
    .be_store = _be_ram_store,
    .be_delete = _be_ram_delete,
};

static conf_backend_t _be_ram =  {
    .ops = &_be_ram_ops,
};

const conf_backend_t *configuration_backend_ram_get(void)
{
    return &_be_ram;
}
