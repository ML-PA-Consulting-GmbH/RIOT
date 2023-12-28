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
 * @file    Test configuration types
 *
 * @author  Fabian Hüßler <fabian.huessler@ml-pa.com>
 */

#ifndef PERSIST_TYPES_H
#define PERSIST_TYPES_H

#include <stdint.h>

#include "container.h"
#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Food configuration item
 */
typedef struct food {
    char price[6];  /**< Food price as a string */
} food_t;

/**
 * @brief   Drinks configuration item
 */
typedef struct drink {
    char price[6];  /**< Drinks price as a string */
} drink_t;

/**
 * @brief   Order configuration item
 */
typedef struct order_item_t {
    char item[12]; /**< Ordered item name */
} order_item_t;

/**
 * @brief   Order configuration
 */
typedef struct order {
    order_item_t items[2];  /**< Ordered items */
} order_t;


#define TEST_CONFIGURATION_MAX_ORDERS   3

/* TODO: automatic generation from .sid file */

#ifndef CONFIG_TEST_FOOD_LOWER_SID
#define CONFIG_TEST_FOOD_LOWER_SID                          (0x7FFFFFFFFFFFFFE0)
#endif
#   ifndef CONFIG_TEST_FOOD_BREAD_LOWER_SID
#   define CONFIG_TEST_FOOD_BREAD_LOWER_SID                 (CONFIG_TEST_FOOD_LOWER_SID + 1)
#   endif
#       ifndef CONFIG_TEST_FOOD_BREAD_WHITE_SID
#       define CONFIG_TEST_FOOD_BREAD_WHITE_SID             (CONFIG_TEST_FOOD_BREAD_LOWER_SID + 1)
#       endif
#       ifndef CONFIG_TEST_FOOD_BREAD_WHOLE_GRAIN_SID
#       define CONFIG_TEST_FOOD_BREAD_WHOLE_GRAIN_SID       (CONFIG_TEST_FOOD_BREAD_LOWER_SID + 2)
#       endif
#   ifndef CONFIG_TEST_FOOD_BREAD_UPPER_SID
#   define CONFIG_TEST_FOOD_BREAD_UPPER_SID                 (CONFIG_TEST_FOOD_BREAD_LOWER_SID + 2)
#   endif
#   ifndef CONFIG_TEST_FOOD_CAKE_LOWER_SID
#   define CONFIG_TEST_FOOD_CAKE_LOWER_SID                  (CONFIG_TEST_FOOD_BREAD_UPPER_SID  + 1)
#   endif
#       ifndef CONFIG_TEST_FOOD_CAKE_CHEESECAKE_SID
#       define CONFIG_TEST_FOOD_CAKE_CHEESECAKE_SID         (CONFIG_TEST_FOOD_CAKE_LOWER_SID + 1)
#       endif
#       ifndef CONFIG_TEST_FOOD_CAKE_DONUT_SID
#       define CONFIG_TEST_FOOD_CAKE_DONUT_SID              (CONFIG_TEST_FOOD_CAKE_LOWER_SID + 2)
#       endif
#   ifndef CONFIG_TEST_FOOD_CAKE_UPPER_SID
#   define CONFIG_TEST_FOOD_CAKE_UPPER_SID                  (CONFIG_TEST_FOOD_CAKE_LOWER_SID + 2)
#   endif
#ifndef CONFIG_TEST_FOOD_UPPER_SID
#define CONFIG_TEST_FOOD_UPPER_SID                          (0x7FFFFFFFFFFFFFE9)
#endif
#ifndef CONFIG_TEST_DRINKS_LOWER_SID
#define CONFIG_TEST_DRINKS_LOWER_SID                        (0x7FFFFFFFFFFFFFEA)
#endif
#   ifndef CONFIG_TEST_DRINKS_COFFEE_SID
#   define CONFIG_TEST_DRINKS_COFFEE_SID                    (CONFIG_TEST_DRINKS_LOWER_SID + 1)
#   endif
#   ifndef CONFIG_TEST_DRINKS_TEA_SID
#   define CONFIG_TEST_DRINKS_TEA_SID                       (CONFIG_TEST_DRINKS_LOWER_SID + 2)
#   endif
#   ifndef CONFIG_TEST_DRINKS_COCOA_SID
#   define CONFIG_TEST_DRINKS_COCOA_SID                     (CONFIG_TEST_DRINKS_LOWER_SID + 3)
#   endif
#ifndef CONFIG_TEST_DRINKS_UPPER_SID
#define CONFIG_TEST_DRINKS_UPPER_SID                        (0x7FFFFFFFFFFFFFF2)
#endif
#ifndef CONFIG_TEST_ORDERS_LOWER_SID
#define CONFIG_TEST_ORDERS_LOWER_SID                        (0x7FFFFFFFFFFFFFF3)                                                        /* /orders */
#endif
#   define CONFIG_TEST_ORDERS_INDEX_LOWER_SID               (1)                                                                         /* /orders[0] */
#   ifndef CONFIG_TEST_ORDERS_INDEX_STRIDE
#   define CONFIG_TEST_ORDERS_INDEX_STRIDE                  (4)                                                                         /* /orders[i] */
#   endif
#       ifndef CONFIG_TEST_ORDERS_ITEMS_LOWER_SID
#       define CONFIG_TEST_ORDERS_ITEMS_LOWER_SID           (CONFIG_TEST_ORDERS_LOWER_SID + CONFIG_TEST_ORDERS_INDEX_LOWER_SID + 1)     /* /orders[0]/items */
#       endif
#           define CONFIG_TEST_ORDERS_ITEMS_INDEX_LOWER_SID (1)                                                                         /* /orders[0]/items[0] */
#           ifndef CONFIG_TEST_ORDERS_ITEMS_INDEX_STRIDE
#           define CONFIG_TEST_ORDERS_ITEMS_INDEX_STRIDE    (1)                                                                         /* /orders[0]/items[i] */
#           endif
#       ifndef CONFIG_TEST_ORDERS_ITEMS_UPPER_SID
#       define CONFIG_TEST_ORDERS_ITEMS_UPPER_SID           (CONFIG_TEST_ORDERS_ITEMS_LOWER_SID + (CONFIG_TEST_ORDERS_ITEMS_INDEX_STRIDE * 2))
#       endif
#ifndef CONFIG_TEST_ORDERS_UPPER_SID
#define CONFIG_TEST_ORDERS_UPPER_SID                        (CONFIG_TEST_ORDERS_LOWER_SID + (CONFIG_TEST_ORDERS_INDEX_STRIDE * TEST_CONFIGURATION_MAX_ORDERS))
#endif

/* ISO C restricts enumerator values to range of ‘int’ */

#define TEST_FOOD_LOWER_SID                                 ((conf_sid_t)CONFIG_TEST_FOOD_LOWER_SID)
#   define TEST_FOOD_BREAD_LOWER_SID                        ((conf_sid_t)CONFIG_TEST_FOOD_BREAD_LOWER_SID)
#       define TEST_FOOD_BREAD_WHITE_SID                    ((conf_sid_t)CONFIG_TEST_FOOD_BREAD_WHITE_SID)
#       define TEST_FOOD_BREAD_WHOLE_GRAIN_SID              ((conf_sid_t)CONFIG_TEST_FOOD_BREAD_WHOLE_GRAIN_SID)
#   define TEST_FOOD_BREAD_UPPER_SID                        ((conf_sid_t)CONFIG_TEST_FOOD_BREAD_UPPER_SID)
#   define TEST_FOOD_CAKE_LOWER_SID                         ((conf_sid_t)CONFIG_TEST_FOOD_CAKE_LOWER_SID)
#       define TEST_FOOD_CAKE_CHEESECAKE_SID                ((conf_sid_t)CONFIG_TEST_FOOD_CAKE_CHEESECAKE_SID)
#       define TEST_FOOD_CAKE_DONUT_SID                     ((conf_sid_t)CONFIG_TEST_FOOD_CAKE_DONUT_SID)
#   define TEST_FOOD_CAKE_UPPER_SID                         ((conf_sid_t)CONFIG_TEST_FOOD_CAKE_UPPER_SID)
#define TEST_FOOD_UPPER_SID                                 ((conf_sid_t)CONFIG_TEST_FOOD_UPPER_SID)
#define TEST_DRINKS_LOWER_SID                               ((conf_sid_t)CONFIG_TEST_DRINKS_LOWER_SID)
#   define TEST_DRINKS_COFFEE_SID                           ((conf_sid_t)CONFIG_TEST_DRINKS_COFFEE_SID)
#   define TEST_DRINKS_TEA_SID                              ((conf_sid_t)CONFIG_TEST_DRINKS_TEA_SID)
#   define TEST_DRINKS_COCOA_SID                            ((conf_sid_t)CONFIG_TEST_DRINKS_COCOA_SID)
#define TEST_DRINKS_UPPER_SID                               ((conf_sid_t)CONFIG_TEST_DRINKS_UPPER_SID)
#define TEST_ORDERS_LOWER_SID                               ((conf_sid_t)CONFIG_TEST_ORDERS_LOWER_SID)
#   define TEST_ORDERS_INDEX_LOWER_SID                      ((conf_sid_t)CONFIG_TEST_ORDERS_INDEX_LOWER_SID)
#   define TEST_ORDERS_INDEX_STRIDE                         ((uint32_t)CONFIG_TEST_ORDERS_INDEX_STRIDE)
#       define TEST_ORDERS_ITEMS_LOWER_SID                  ((conf_sid_t)CONFIG_TEST_ORDERS_ITEMS_LOWER_SID)
#           define TEST_ORDERS_ITEMS_INDEX_LOWER_SID        ((conf_sid_t)CONFIG_TEST_ORDERS_ITEMS_INDEX_LOWER_SID)
#           define TEST_ORDERS_ITEMS_INDEX_STRIDE           ((uint32_t)CONFIG_TEST_ORDERS_ITEMS_INDEX_STRIDE)
#       define TEST_ORDERS_ITEMS_UPPER_SID                  ((conf_sid_t)CONFIG_TEST_ORDERS_ITEMS_UPPER_SID)
#define TEST_ORDERS_UPPER_SID                               ((conf_sid_t)CONFIG_TEST_ORDERS_UPPER_SID)

/**
 * @brief   Full configuration compound item
 */
struct configuration {
    struct {
        struct {
            food_t white;                           /**< location of "/food/bread/white" */
            food_t whole_grain;                     /**< location of "/food/bread/whole_grain" */
        } bread;                                    /**< location of "/food/bread" */
        struct {
            food_t cheesecake;                      /**< location of "/food/cake/cheesecake" */
            food_t donut;                           /**< location of "/food/cake/donut" */
        } cake;                                     /**< location of "/food/cake" */
    } food;                                         /**< location of "/food" */
    struct {
        drink_t coffee;                             /**< location of "/drinks/coffee" */
        drink_t tea;                                /**< location of "/drinks/tea" */
        drink_t cocoa;                              /**< location of "/drinks/cocoa" */
    } drinks;                                       /**< location of "/drinks" */
    order_t orders[TEST_CONFIGURATION_MAX_ORDERS];  /**< location of "/orders" */
};

#ifdef __cplusplus
}
#endif

#endif /* PERSIST_TYPES_H */
/** @} */
