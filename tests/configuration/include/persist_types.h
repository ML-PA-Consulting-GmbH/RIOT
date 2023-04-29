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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Number of "/food/bread" items to store
 */
#define CONF_FOOD_BREAD_NUMOF   2
/**
 * @brief   Number of "/food/cake" items to store
 */
#define CONF_FOOD_CAKE_NUMOF    2
/**
 * @brief   Number of "/drinks" items to store
 */
#define CONF_DRINKS_NUMOF       3

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
 * @brief   Full configuration compound item
 */
struct configuration {
    struct {
        food_t food_bread[CONF_FOOD_BREAD_NUMOF];   /**< location of "/food/bread "*/
        food_t food_cake[CONF_FOOD_CAKE_NUMOF];     /**< location of "/food/drinks" */
    } food;                                         /**< location of "/food" */
    drink_t drinks[CONF_DRINKS_NUMOF];              /**< location of "/drinks" */
};

#ifdef __cplusplus
}
#endif

#endif /* PERSIST_TYPES_H */
/** @} */
