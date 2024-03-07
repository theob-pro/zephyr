/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

/** @typedef bt_gatt_write_func_t
 *  @brief Write callback function
 *
 *  @param conn Connection object.
 *  @param err ATT error code.
 *  @param params Write parameters used.
 */
typedef void (*bt_gatt_write_func_t)(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_write_params *params);

int bt_gatt_write(struct bt_conn *conn, enum bt_att_chan_opt chan_opt, uint16_t handle,
			 const void *data, uint16_t length, bt_gatt_write_func_t cb,
			 struct simple_write_mem *mem);
int bt_gatt_write_long(struct bt_conn *conn, enum bt_att_chan_opt chan_opt, uint16_t handle,
		       uint16_t offset, const void *data, uint16_t length,
		       struct mem_for_long_write_operation *mem);
