/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_central, LOG_LEVEL_DBG);

#include "bs_bt_utils.h"

void central(void)
{
	int err;
	struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {};

	bt_conn_auth_info_cb.pairing_failed = pairing_failed;
	bt_conn_auth_info_cb.pairing_complete = pairing_complete;

	LOG_DBG("===== Central =====");

	err = bt_enable(NULL);
	ASSERT(!err, "bt_enable failed (%d)\n", err);

	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	err = settings_load();
	ASSERT(!err, "settings_load failed (%d)\n", err);

	scan_connect_to_first_result();
	wait_connected();

	set_security(BT_SECURITY_L2);

	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);

	/* TODO: subscribe to a service changed indication */
	// subscribe();

	disconnect();
	wait_disconnected();
	clear_g_conn();

	scan_connect_to_first_result();
	wait_connected();

	/* TODO: wait for notification? but why? if there is no security it's
	 *       obvious we'll not receive one. so wait for sec update
         */

	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);

	/* test will timeout if security is not restored */

	PASS("PASS\n");
}
