/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>

#include "signal.h"

struct gatt_service_discovery {
	int api_err;
	/* The rest is valid if `api_err == 0`. */
	uint8_t att_err;
	/* The rest is valid if `att_err == 0`. */
	uint16_t start_handle;
	uint16_t end_handle;
};

struct gatt_discover_closure {
	struct bt_conn *conn;
	struct bt_gatt_discover_params params;
	struct gatt_service_discovery *result;
	struct k_poll_signal signal;
};

/* bt_gatt_discover_func_t */
static inline uint8_t gatt_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       struct bt_gatt_discover_params *params)
{
	struct gatt_discover_closure *ctx =
		CONTAINER_OF(params, struct gatt_discover_closure, params);

	ctx->result->att_err = attr ? 0 : BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	if (attr) {
		ctx->result->att_err = 0;
		ctx->result->start_handle = attr->handle;
		ctx->result->end_handle =
			((struct bt_gatt_service_val *)attr->user_data)->end_handle;
	}

	k_poll_signal_raise(&ctx->signal, 0);
	return BT_GATT_ITER_STOP;
}

static inline void bt_util_sync_bt_gatt_discover(struct gatt_discover_closure *ctx)
{
	int err;

	k_poll_signal_init(&ctx->signal);

	err = bt_gatt_discover(ctx->conn, &ctx->params);
	ctx->result->api_err = err;

	if (!err) {
		bt_util_await_signal(&ctx->signal);
	}
}

static inline void bt_util_gatt_discover_primary_service_sync(struct gatt_service_discovery *result,
							      struct bt_conn *conn,
							      const struct bt_uuid *service_type)
{
	struct gatt_discover_closure ctx = {
		.conn = conn,
		.result = result,
		.params = {.type = BT_GATT_DISCOVER_PRIMARY,
			   .start_handle = 0x0001,
			   .end_handle = 0xffff,
			   .uuid = service_type,
			   .func = gatt_discover_cb},
	};

	bt_util_sync_bt_gatt_discover(&ctx);
}
