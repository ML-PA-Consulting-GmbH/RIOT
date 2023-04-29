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
#include <stdio.h>
#include <string.h>

#include "configuration.h"

#include "container.h"
#include "persist_types.h"

struct configuration persist_conf = {
    .food = {
        .food_bread = {
            {"1.00"},
            {"1.20"},
        },
        .food_cake = {
            {"1.99"},
            {"1.00"},
        },
    },
    .drinks = {
        {"0.50"},
        {"0.60"},
        {"1.00"},
    },
};

struct kv {
    const char *key;
    void *value;
};

static struct kv _kv[] = {
    {CONFIGURATION_RIOT_ROOT"/food/bread/white", &persist_conf.food.food_bread[0]},
    {CONFIGURATION_RIOT_ROOT"/food/bread/whole grain", &persist_conf.food.food_bread[1]},

    {CONFIGURATION_RIOT_ROOT"/food/cake/cheesecake", &persist_conf.food.food_cake[0]},
    {CONFIGURATION_RIOT_ROOT"/food/cake/donut", &persist_conf.food.food_cake[1]},

    {CONFIGURATION_RIOT_ROOT"/drinks/coffee", &persist_conf.drinks[0]},
    {CONFIGURATION_RIOT_ROOT"/drinks/tea", &persist_conf.drinks[1]},
    {CONFIGURATION_RIOT_ROOT"/drinks/cocoa", &persist_conf.drinks[2]},
};

static int _be_ram_load(const struct conf_backend *be,
                        conf_key_buf_t *key, void *val, size_t *size)
{
    (void)be;
    for (unsigned i = 0; i < ARRAY_SIZE(_kv); i++) {
        if (!strcmp(key->buf, _kv[i].key)) {
            memcpy(val, _kv[i].value, *size);
            return 0;
        }
    }
    return -ENOENT;
}

static int _be_ram_store(const struct conf_backend *be,
                         conf_key_buf_t *key, const void *val, size_t *size,
                         off_t part_offset, size_t part_size)
{
    (void)be; (void)part_offset; (void)part_size;
    for (unsigned i = 0; i < ARRAY_SIZE(_kv); i++) {
        if (!strcmp(key->buf, _kv[i].key)) {
            memcpy(_kv[i].value, val, *size);
            return 0;
        }
    }
    return -ENOENT;
}

static const conf_backend_ops_t _be_ram_ops = {
    .be_load = _be_ram_load,
    .be_store = _be_ram_store,
};

static conf_backend_t _be_ram =  {
    .ops = &_be_ram_ops,
};

const conf_backend_t *configuration_backend_ram_get(void)
{
    return &_be_ram;
}
