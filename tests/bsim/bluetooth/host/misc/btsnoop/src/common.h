/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#include "bs_tracing.h"
#include "bstests.h"

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

extern enum bst_result_t bst_result;

#define CREATE_FLAG(flag) static atomic_t flag = (atomic_t) false
#define SET_FLAG(flag)	  (void)atomic_set(&flag, (atomic_t) true)
#define GET_FLAG(flag)	  (bool)atomic_get(&flag)
#define UNSET_FLAG(flag)  (void)atomic_set(&flag, (atomic_t) false)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}

LOG_MODULE_DECLARE(bt_bsim_btsnoop, LOG_LEVEL_DBG);
