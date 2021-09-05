/*
 * Copyright (C) 2017-2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_cord_config CoRE RD Configuration
 * @ingroup     net_cord
 * @brief       Configuration options for CoRE RD endpoints and lookup clients
 * @{
 *
 * @file
 * @brief       (Default) configuration values for CoRE RD endpoints and lookup
 *              clients
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef NET_CORD_CONFIG_H
#define NET_CORD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup net_cord_conf CoRE RD Client compile configurations
 * @ingroup config
 * @{
 */
/**
 * @brief   Default lifetime in seconds (the default is 1 day)
 */
#ifndef CONFIG_CORD_LT
#define CONFIG_CORD_LT                 (86400UL)
#endif

/**
 * @brief   Default client update interval (default is 3/4 the lifetime)
 */
#ifndef CONFIG_CORD_UPDATE_INTERVAL
#define CONFIG_CORD_UPDATE_INTERVAL    ((CONFIG_CORD_LT / 4) * 3)
#endif
/** @} */

/**
 * @brief   Delay until the RD client starts to try registering (in seconds)
 */
#ifndef CORD_STARTUP_DELAY
#define CORD_STARTUP_DELAY      (3U)
#endif

/**
 * @name    Endpoint ID definition
 *
 * Per default, the endpoint ID (ep) is generated by concatenation of a user
 * defined prefix @ref CORD_EP_PREFIX and a locally unique ID (luid) encoded in
 * hexadecimal formatting with the given length of characters
 * @ref CORD_EP_SUFFIX_LEN.
 *
 * Alternatively, the endpoint ID value can be defined at compile time by
 * assigning a string value to the @ref CONFIG_CORD_EP macro.
 *
 * @{
 */
#ifndef CONFIG_CORD_EP
#ifdef DOXYGEN
/**
 * @ingroup net_cord_conf
 * @brief Endpoint ID definition
 * @{
 */
#define CONFIG_CORD_EP "MyNewEpName"    //defined for doxygen documentation only
#endif
/** @} */

/**
 * @brief   Number of generated hexadecimal characters added to the ep
 *
 * @note    Must be an even number
 */
#define CORD_EP_SUFFIX_LEN      (16)

/**
 * @brief   Default static prefix used for the generated ep
 */
#define CORD_EP_PREFIX          "RIOT-"
#endif
/** @} */

/**
 * @brief   Extra query parameters added during registration
 *
 * Must be suitable for constructing a static array out of them.
 *
 * Example:
 *
 * ```
 * CFLAGS += '-DCONFIG_CORD_EXTRAARGS="proxy=on","et=tag:riot-os.org,2020:board"'
 * ```
 */
#ifdef DOXYGEN
#define CONFIG_CORD_EXTRAARGS
#endif

#ifdef __cplusplus
}
#endif

#endif /* NET_CORD_CONFIG_H */
/** @} */
