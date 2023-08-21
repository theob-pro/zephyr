/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#include "utils.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tester, LOG_LEVEL_DBG);

DEFINE_FLAG(is_connected);

static ssize_t test_on_attr_read_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	/* Do not respond until allowed */
	static int i = 0;
	if (!i) {
		LOG_DBG("Sleeping");
		k_sleep(K_SECONDS(20));
	}

	LOG_ERR("handled read %i", i++);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, "data", 4);
}

BT_GATT_SERVICE_DEFINE(test_gatt_service, BT_GATT_PRIMARY_SERVICE(test_service_uuid),
		       BT_GATT_CHARACTERISTIC(test_characteristic_uuid, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, test_on_attr_read_cb, NULL, NULL));

static struct bt_conn *default_conn;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	LOG_DBG("%s", addr);

	default_conn = bt_conn_ref(conn);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%p %s (reason 0x%02x)", conn, addr, reason);

	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void adv(void)
{
	UNSET_FLAG(is_connected);

	int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, NULL, 0, NULL, 0);

	ASSERT(!err, "Advertising failed to start (err %d)\n", err);

	LOG_DBG("Waiting for Central...");
}

void test_procedure_tester(void)
{
	LOG_DBG("Deadlock tester/peripheral started*");
	int err;

	err = bt_enable(NULL);
	ASSERT(err == 0, "(err %d)\n", err);
	LOG_DBG("Tester Bluetooth initialized.");

	adv();
	WAIT_FOR_FLAG(is_connected);

	k_sleep(K_SECONDS(5));

	WAIT_FOR_FLAG_UNSET(is_connected);

	PASS("Tester done\n");
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "tester",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_procedure_tester,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
