/*
 * Copyright (C) 2020 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 */
#include <stdint.h>
#include <assert.h>

#include "embUnit/embUnit.h"

#include "unittests-constants.h"

/* mockup */

static uint64_t now;

static inline uint64_t xtimer_now_usec64(void)
{
    return now;
}

#include "sdcard_spi_internal.h"

/* common variables */

int32_t retry_value;
uint64_t retry_cmp;

/* tests */

static void set_up(void)
{
    now = 0;
    retry_value = 0;
    retry_cmp = 0;
}

static void test_retry_init_zero(void)
{
    retry_cmp = TEST_UINT64;

    _retry_init(retry_value, &retry_cmp);

    TEST_ASSERT_EQUAL_INT(0, retry_cmp);
}

static void test_retry_init_counting(void)
{
    retry_value = 123;

    _retry_init(retry_value, &retry_cmp);

    TEST_ASSERT_EQUAL_INT(123, retry_cmp);
}

static void test_retry_init_timeout(void)
{
    now = 7;
    retry_value = -123;

    _retry_init(retry_value, &retry_cmp);

    TEST_ASSERT_EQUAL_INT(7 + 123, retry_cmp);
}

static void test_retry_process_zero(void)
{
    _retry_init(retry_value, &retry_cmp);

    TEST_ASSERT(!_retry_process(&retry_value, retry_cmp));
    TEST_ASSERT(!_retry_process(&retry_value, retry_cmp)); /* rollover test */
    TEST_ASSERT_EQUAL_INT(0, retry_value);
}

static void test_retry_process_counting(void)
{
    int ii;

    retry_value = 4;

    _retry_init(retry_value, &retry_cmp);

    for (ii = 0; ii < 4; ii++) {
        TEST_ASSERT(_retry_process(&retry_value, retry_cmp));
    }
    TEST_ASSERT(!_retry_process(&retry_value, retry_cmp));
    TEST_ASSERT(!_retry_process(&retry_value, retry_cmp)); /* rollover test */
    TEST_ASSERT_EQUAL_INT(0, retry_value);
}

static void test_retry_process_timeout(void)
{
    int ii;

    now = 7;
    retry_value = -21;

    _retry_init(retry_value, &retry_cmp);

    for (ii = 0; ii < 7; ii++) {
        TEST_ASSERT(_retry_process(&retry_value, retry_cmp));
        now += 3;
    }
    TEST_ASSERT(!_retry_process(&retry_value, retry_cmp));

    /* exact timeout match */
    now = 7 + 21;
    TEST_ASSERT(!_retry_process(&retry_value, retry_cmp));
}

static void test_retry_elapsed_zero(void)
{
    _retry_init(retry_value, &retry_cmp);

    TEST_ASSERT(0ULL == _retry_elapsed(retry_value, retry_cmp));
}

static void test_retry_elapsed_counting(void)
{
    retry_value = 123;

    _retry_init(retry_value, &retry_cmp);
    _retry_process(&retry_value, retry_cmp);
    _retry_process(&retry_value, retry_cmp);

    TEST_ASSERT(2ULL == _retry_elapsed(retry_value, retry_cmp));
}

static void test_retry_elapsed_timeout(void)
{
    now = 13;
    retry_value = -123;

    _retry_init(retry_value, &retry_cmp);
    now = 63;

    TEST_ASSERT(50ULL == _retry_elapsed(retry_value, retry_cmp));
}

Test *tests_sdcard_spi_internal_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_retry_init_zero),
        new_TestFixture(test_retry_init_counting),
        new_TestFixture(test_retry_init_timeout),
        new_TestFixture(test_retry_process_zero),
        new_TestFixture(test_retry_process_counting),
        new_TestFixture(test_retry_process_timeout),
        new_TestFixture(test_retry_elapsed_zero),
        new_TestFixture(test_retry_elapsed_counting),
        new_TestFixture(test_retry_elapsed_timeout),
    };

    EMB_UNIT_TESTCALLER(sdcard_spi_internal_tests, set_up, NULL, fixtures);

    return (Test *)&sdcard_spi_internal_tests;
}

void tests_sdcard_spi_internal(void)
{
    TestRunner_runTest(tests_sdcard_spi_internal_tests());
}
/** @} */
