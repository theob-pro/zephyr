/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "signal.h"

struct bt_util_att_read {
	int api_err;
	/* The rest is valid if `api_err == 0`. */
	uint8_t att_err;
	/* The rest is valid if `att_err == 0`. */
	uint16_t handle;
	uint16_t read_len; /* Input: Size of `read_buf`. Output: Read length. */
	uint8_t *read_buf; /* Output. Can be NULL. */
};

struct att_read_closure {
	struct bt_conn *conn;
	struct bt_gatt_read_params params;
	struct bt_util_att_read *result;
	struct k_poll_signal signal;
};

static inline uint8_t att_read_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t read_len)
{
	struct att_read_closure *ctx = CONTAINER_OF(params, struct att_read_closure, params);

	ctx->result->att_err = att_err;
	ctx->result->handle = params->by_uuid.start_handle;
	ctx->result->read_len = read_len;
	if (!att_err && ctx->result->read_buf) {
		__ASSERT_NO_MSG(read_len <= ctx->result->read_len);
		memcpy(ctx->result->read_buf, data, read_len);
	}

	k_poll_signal_raise(&ctx->signal, 0);
	return BT_GATT_ITER_STOP;
}

static inline void bt_util_sync_bt_gatt_read(struct att_read_closure *ctx)
{
	int err;

	k_poll_signal_init(&ctx->signal);

	err = bt_gatt_read(ctx->conn, &ctx->params);
	ctx->result->api_err = err;

	if (!err) {
		bt_util_await_signal(&ctx->signal);
	}
}

static inline void bt_util_att_read_by_type_sync(struct bt_util_att_read *result,
						 struct bt_conn *conn, enum bt_att_chan_opt bearer,
						 const struct bt_uuid *type, uint16_t start_handle,
						 uint16_t end_handle)
{
	struct att_read_closure ctx = {
		.conn = conn,
		.result = result,
		.params = {.by_uuid = {.uuid = type,
				       .start_handle = start_handle,
				       .end_handle = end_handle},
			   .func = att_read_cb,
			   IF_ENABLED(CONFIG_BT_EATT, (.chan_opt = bearer))},
	};

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	bt_util_sync_bt_gatt_read(&ctx);
}

static inline void bt_util_att_read_by_handle(struct bt_util_att_read *result, struct bt_conn *conn,
					      enum bt_att_chan_opt bearer, uint16_t handle,
					      uint16_t offset)
{
	struct att_read_closure ctx = {
		.conn = conn,
		.result = result,
		.params = {.handle_count = 1,
			   .single = {.handle = handle, .offset = offset},
			   .func = att_read_cb,
			   IF_ENABLED(CONFIG_BT_EATT, (.chan_opt = bearer))},
	};

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	bt_util_sync_bt_gatt_read(&ctx);
}
