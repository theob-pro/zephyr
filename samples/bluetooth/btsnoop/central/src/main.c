/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

// #include <zephyr/logging/log.h>
// LOG_MODULE_DECLARE(btsnoop_sample_central, LOG_LEVEL_DBG);

static struct bt_conn *default_conn;

static void device_found(const bt_addr_le_t *addr,
			 int8_t rssi,
			 uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	// LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

	err = bt_le_scan_stop();
	// LOG_DBG("bt_le_scan_stop %d", err);
	__ASSERT_NO_MSG(err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	// LOG_DBG("bt_conn_le_create %d", err);
	__ASSERT_NO_MSG(err);
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	// LOG_DBG("bt_le_scan_start %d", err);
	__ASSERT_NO_MSG(err);

	// LOG_DBG("Scanner successfully started");
}

int main(void)
{
	int err;

	// LOG_DBG("Starting central sample...");

	err = bt_enable(NULL);
	// LOG_DBG("bt_enable %d", err);
	__ASSERT_NO_MSG(err);

	// LOG_DBG("Bluetooth initialised");

	start_scan();

	return 0;
}
