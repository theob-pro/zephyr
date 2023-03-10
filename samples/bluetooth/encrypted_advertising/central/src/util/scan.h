/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>

#include "signal.h"

struct bt_util_scan_find_name {
	int api_err;
	/* The rest is valid if `api_err == 0`. */
	bt_addr_le_t addr;
};

struct bt_util_scan_find_name_closure {
	char *wanted_name;
	struct bt_util_scan_find_name *result;
	struct k_poll_signal signal;
};

K_MUTEX_DEFINE(g_ctx_lock);
struct bt_util_scan_find_name_closure *g_ctx;

static bool bt_scan_find_name_cb_data_cb(struct bt_data *data, void *user_data)
{
	char *name_matched = user_data;

	if (data->type != BT_DATA_NAME_COMPLETE && data->type != BT_DATA_NAME_SHORTENED) {
		/* Continue with next ad data. */
		return true;
	}

	if (data->data_len == strlen(g_ctx->wanted_name) &&
	    !memcmp(g_ctx->wanted_name, data->data, data->data_len)) {
		*name_matched = true;
	}

	return false;
}

void bt_scan_find_name_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			  struct net_buf_simple *buf)
{
	bool name_matched = false;

	bt_data_parse(buf, bt_scan_find_name_cb_data_cb, &name_matched);

	if (!name_matched) {
		/* Continue with next adv report. */
		return;
	}

	g_ctx->result->addr = *addr;
	(void)bt_le_scan_stop();
	k_poll_signal_raise(&g_ctx->signal, 0);
}

static inline void bt_util_scan_find_name(struct bt_util_scan_find_name *result, char name[])
{
	int err;
	struct bt_util_scan_find_name_closure ctx = {
		.wanted_name = name,
		.result = result,
	};

	k_poll_signal_init(&ctx.signal);
	k_mutex_lock(&g_ctx_lock, K_FOREVER);
	g_ctx = &ctx;
	compiler_barrier();
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, bt_scan_find_name_cb);
	if (!err) {
		bt_util_await_signal(&ctx.signal);
	}
	k_mutex_unlock(&g_ctx_lock);

	result->api_err = err;
}
