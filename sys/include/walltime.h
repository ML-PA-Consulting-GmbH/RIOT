/*
 * Copyright (C) 2025 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    sys_walltime    wall-clock time helper functions
 * @ingroup     sys
 * @brief       Common functions to access the wall-clock / real time clock
 * @{
 * @file
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#ifndef WALLTIME_H
#define WALLTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * @brief   Set the system date / time
 *
 * @param[in] time  The current data / time to set
 */
void walltime_set(struct tm *time);

/**
 * @brief   Get the system date / time
 *
 * @param[out] time current time output
 * @param[out] ms   current milliseconds output, may be NULL
 *                  resolution depends on the backend
 */
void walltime_get(struct tm *time, uint16_t *ms);

/**
 * @brief   Get the current system time in seconds since @see RIOT_EPOCH
 *
 * @param[out] ms   current milliseconds output, may be NULL
 *                  resolution depends on the backend
 *
 * @returns seconds since `RIOT_EPOCH`
 */
uint32_t walltime_get_riot(uint16_t *ms);

/**
 * @brief   Get the current system time in seconds since 01.01.1970
 *
 * @param[out] ms   current milliseconds output, may be NULL
 *                  resolution depends on the backend
 *
 * @returns seconds since 01.01.1970
 */
time_t walltime_get_unix(uint16_t *ms);

/**
 * @brief   Get seconds elapsed since last reset
 *
 * @param[in] full  set to false to get seconds since last (warm) boot / wake-up
 *                  set to true to get seconds since last cold boot / full reset
 *
 */
uint32_t walltime_uptime(bool full);

#ifdef __cplusplus
}
#endif

#endif /* WALLTIME_H */
/** @} */
