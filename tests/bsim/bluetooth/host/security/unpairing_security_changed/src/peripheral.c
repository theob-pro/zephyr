/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/toolchain/gcc.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "bs_bt_utils.h"

LOG_MODULE_REGISTER(test_peripheral, LOG_LEVEL_DBG);

BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");

static bool unpair = false;

static void peripheral_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	/* Try to trigger fault here */
	k_msleep(2000);
	LOG_ERR("do bad");
	if (unpair) {
		bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
		LOG_DBG("unpaired");
	} else {
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

void peripheral(void)
{
	LOG_DBG("===== Peripheral =====");

	struct bt_conn_cb peripheral_cb = {};

	peripheral_cb.security_changed = peripheral_security_changed;

	bs_bt_utils_setup();

	bt_conn_cb_register(&peripheral_cb);

	LOG_DBG("Peripheral security changed callback set");

	/* Disconnect in the security changed callback */

	advertise_connectable(BT_ID_DEFAULT, NULL);
	wait_connected();

	wait_disconnected();

	clear_g_conn();
	bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);

	/* Call `bt_unpair` in security changed callback */

	unpair = true;

	advertise_connectable(BT_ID_DEFAULT, NULL);
	wait_connected();

	wait_disconnected();

	clear_g_conn();

	PASS("PASS\n");
}
