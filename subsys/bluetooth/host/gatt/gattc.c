/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gattc.h>

#include "../att_internal.h"
#include "../conn_internal.h"

static struct bt_att_req *gatt_req_alloc(bt_att_func_t func, void *params,
					 bt_att_encode_t encode,
					 uint8_t op,
					 size_t len)
{
	struct bt_att_req *req;

	/* Allocate new request */
	req = bt_att_req_alloc(BT_ATT_TIMEOUT);
	if (!req) {
		return NULL;
	}

#if defined(CONFIG_BT_SMP)
	req->att_op = op;
	req->len = len;
	req->encode = encode;
#endif
	req->func = func;
	req->user_data = params;

	return req;
}

static int gatt_req_send(struct bt_conn *conn, bt_att_func_t func, void *params,
			 bt_att_encode_t encode, uint8_t op, size_t len,
			 enum bt_att_chan_opt chan_opt)

{
	struct bt_att_req *req;
	struct net_buf *buf;
	int err;

	if (IS_ENABLED(CONFIG_BT_EATT) &&
	    !bt_att_chan_opt_valid(conn, chan_opt)) {
		return -EINVAL;
	}

	req = gatt_req_alloc(func, params, encode, op, len);
	if (!req) {
		return -ENOMEM;
	}

	buf = bt_att_create_pdu(conn, op, len);
	if (!buf) {
		bt_att_req_free(req);
		return -ENOMEM;
	}

	bt_att_set_tx_meta_data(buf, NULL, NULL, chan_opt);

	req->buf = buf;

	err = encode(buf, len, params);
	if (err) {
		bt_att_req_free(req);
		return err;
	}

	err = bt_att_req_send(conn, req);
	if (err) {
		bt_att_req_free(req);
	}

	return err;
}

static int gatt_write_encode(struct net_buf *buf, size_t len, void *user_data)
{
	struct bt_gatt_write_params *params = user_data;
	struct bt_att_write_req *req;
	size_t write;

	req = net_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(params->handle);

	write = net_buf_append_bytes(buf, params->length, params->data,
				     K_NO_WAIT, NULL, NULL);
	if (write != params->length) {
		return -ENOMEM;
	}

	return 0;
}

static void gatt_write_rsp(struct bt_conn *conn, int err, const void *pdu,
			   uint16_t length, void *user_data)
{
	struct bt_gatt_write_params *params = user_data;

	LOG_DBG("err %d", err);

	params->func(conn, att_err_from_int(err), params);
}

int bt_gatt_write(struct bt_conn *conn, enum bt_att_chan_opt chan_opt, uint16_t handle,
			 const void *data, uint16_t length, bt_gatt_write_func_t cb,
			 struct simple_write_mem *mem)
{
	size_t len;
	struct bt_gatt_write_params *params;

#if !defined(CONFIG_BT_EATT)
	ARG_UNUSED(chan_opt);
#endif
	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(mem, "invalid parameters\n");
	__ASSERT(cb, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	len = sizeof(struct bt_att_write_req) + length;

	if (len > (bt_att_get_mtu(conn) - 1)) {
		LOG_DBG("data are too large");
		return -EDOM;
	}

	params = (struct bt_gatt_write_params *)mem->_simple_write_data;

	params->func = cb;
	params->data = data;
	params->length = length;
	params->handle = handle;
#if defined(CONFIG_BT_EATT)
	params->chan_opt = chan_opt;
#endif

	return gatt_req_send(conn, gatt_write_rsp, params, gatt_write_encode, BT_ATT_OP_WRITE_REQ,
			     len, BT_ATT_CHAN_OPT(params));
}

int bt_gatt_write_long(struct bt_conn *conn, enum bt_att_chan_opt chan_opt, uint16_t handle,
		       uint16_t offset, const void *data, uint16_t length, bt_gatt_write_func_t cb,
		       struct write_long_mem *mem)
{
#if !defined(CONFIG_BT_EATT)
	ARG_UNUSED(chan_opt);
#endif
	__ASSERT(conn, "invalid parameters\n");
	__ASSERT(mem, "invalid parameters\n");
	__ASSERT(cb, "invalid parameters\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	struct bt_gatt_write_params *params = (struct bt_gatt_write_params *)mem->_write_long_data;

	params->func = cb;
	params->data = data;
	params->length = length;
	params->handle = handle;
	params->offset = offset;
#if defined(CONFIG_BT_EATT)
	params->chan_opt = chan_opt;
#endif

	return gatt_prepare_write(conn, params);
}
