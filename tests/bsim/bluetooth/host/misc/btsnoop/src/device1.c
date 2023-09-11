/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "common.h"

static void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_CONNECTABLE;
	params.options |= BT_LE_ADV_OPT_EXT_ADV;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	params.interval_max = BT_GAP_ADV_SLOW_INT_MAX;

	err = bt_le_ext_adv_create(&params, NULL, adv);
	if (err) {
		FAIL("Failed to create advertiser (%d)\n", err);
	}
}

static void start_adv(struct bt_le_ext_adv *adv)
{
	int err;
	int32_t timeout = 0;
	uint8_t num_events = 0;

	struct bt_le_ext_adv_start_param start_params;

	start_params.timeout = timeout;
	start_params.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &start_params);
	if (err) {
		FAIL("Failed to start advertiser (%d)\n", err);
	}

	LOG_DBG("Advertiser started");
}

void test_device1(void)
{
	int err;
	struct bt_le_ext_adv *adv = NULL;

	LOG_DBG("===== Device 1 =====");

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
	}

	LOG_DBG("Bluetooth initialized");

	create_adv(&adv);
	start_adv(adv);

	k_msleep(2000);

	PASS("PASS\n");
}
