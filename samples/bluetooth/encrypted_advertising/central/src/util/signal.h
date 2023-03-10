/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifndef __EAD_UTIL_SIGNAL_H
#define __EAD_UTIL_SIGNAL_H

static inline void bt_util_await_signal(struct k_poll_signal *sig)
{
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, sig),
	};

	k_poll(events, ARRAY_SIZE(events), K_FOREVER);
}

#endif /* __EAD_UTIL_SIGNAL_H */
