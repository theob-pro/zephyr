/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

// #include <zephyr/logging/log.h>
// LOG_MODULE_DECLARE(btsnoop_sample_peripheral, CONFIG_BT_EAD_LOG_LEVEL);

static void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_CONNECTABLE;
	params.options |= BT_LE_ADV_OPT_EXT_ADV;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	err = bt_le_ext_adv_create(&params, NULL, adv);
	// LOG_DBG("bt_le_ext_adv_create %d", err);
	__ASSERT_NO_MSG(err);
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
	// LOG_DBG("bt_le_ext_adv_start %d", err);
	__ASSERT_NO_MSG(err);

	// LOG_DBG("Advertiser started");
}

int main(void)
{
	int err;
	struct bt_le_ext_adv *adv = NULL;

	// LOG_DBG("Starting peripheral sample...");

	err = bt_enable(NULL);
	// LOG_DBG("bt_enable %d", err);
	__ASSERT_NO_MSG(err);

	// LOG_DBG("Bluetooth initialised");

	create_adv(&adv);

	start_adv(adv);

	return 0;
}
