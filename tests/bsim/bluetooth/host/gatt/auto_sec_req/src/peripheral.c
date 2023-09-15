/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_peripheral, LOG_LEVEL_DBG);

#include "bs_bt_utils.h"

void peripheral(void)
{
	LOG_DBG("===== Peripheral =====");

	int err;
	struct bt_le_ext_adv *adv = NULL;

	err = bt_enable(NULL);
	ASSERT(!err, "bt_enable failed (%d)\n", err);

	err = settings_load();
	ASSERT(!err, "settings_load failed (%d)\n", err);

	create_adv(&adv);
	start_adv(adv);
	wait_connected();

	stop_adv(adv);

	wait_disconnected();
	clear_g_conn();

	/* TODO: change something in GATT database to trigger the service changed indication */

	start_adv(adv);
	wait_connected();

	PASS("PASS\n");
}
