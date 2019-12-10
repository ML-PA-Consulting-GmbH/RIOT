/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     drivers_lis2dh12
 * @{
 *
 * @file
 * @brief       LIS2DH12 accelerometer SAUL mapping
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include "saul.h"
#include "lis2dh12.h"

static int read_accelerometer(const void *dev, phydat_t *res)
{
    if (lis2dh12_read((const lis2dh12_t *)dev, res->val) != LIS2DH12_OK) {
        return 0;
    }
    res->unit = UNIT_G;
    res->scale = -3;
    return 3;
}

static int write_accelerometer(const void *dev, phydat_t *data)
{
    /* Using non-const dev !! */
    (void) dev;
    char str[6] = {0};
    sprintf(str,"0x%02X",data->val[0]);
    puts("String: ");
    puts(str);
    sprintf(str,"0x%02X",data->val[1]);
    puts(str);

    if (lis2dh12_write((const lis2dh12_t *)dev, data->val[0], data->val[1]) != LIS2DH12_OK) {
        return 0;
    }
    return 3;
}

const saul_driver_t lis2dh12_saul_driver = {
    .read = read_accelerometer,
    .write = write_accelerometer,
    .type = SAUL_SENSE_ACCEL
};
