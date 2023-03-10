/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/ead.h>

#ifndef __EAD_SAMPLE_COMMON_H
#define __EAD_SAMPLE_COMMON_H

#define DEFINE_FLAG(flag) atomic_t flag = (atomic_t) false
#define SET_FLAG(flag)	  (void)atomic_set(&flag, (atomic_t) true)
#define UNSET_FLAG(flag)  (void)atomic_set(&flag, (atomic_t) false)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}

struct material_key {
	uint8_t session_key[BT_EAD_KEY_SIZE];
	uint8_t iv[BT_EAD_IV_SIZE];
} __packed;

#define CUSTOM_SERVICE_TYPE BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0)
#define BT_UUID_CUSTOM_SERVICE BT_UUID_DECLARE_128(CUSTOM_SERVICE_TYPE)

#endif /* __EAD_SAMPLE_COMMON_H */
