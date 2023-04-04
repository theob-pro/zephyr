/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci_err.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>

#include <zephyr/settings/settings.h>

#include "common.h"
#include "settings.h"

CREATE_FLAG(connected_flag);
CREATE_FLAG(disconnected_flag);
CREATE_FLAG(security_updated_flag);

CREATE_FLAG(notif_received_flag);
CREATE_FLAG(ccc_subscribed_flag);
CREATE_FLAG(ccc_unsubscribed_flag);

CREATE_FLAG(gatt_discover_done_flag);

#define BT_UUID_DUMMY_SERVICE BT_UUID_DECLARE_128(DUMMY_SERVICE_TYPE)
#define BT_UUID_DUMMY_SERVICE_NOTIFY BT_UUID_DECLARE_128(DUMMY_SERVICE_NOTIFY_TYPE)

static struct bt_conn *default_conn;

static struct bt_conn_cb central_cb;

static struct bt_gatt_discover_params disc_params;
static uint16_t ccc_handle;
static uint16_t value_handle;
static struct bt_gatt_subscribe_params sub_params;
static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);

static const struct bt_uuid *ccc_uuid = BT_UUID_GATT_CCC;

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	SET_FLAG(notif_received_flag);

        if (data == NULL) {
		SET_FLAG(ccc_unsubscribed_flag);
                LOG_DBG("[NOTIFICATION] data is NULL");
		LOG_INF("[UNSUBSCRIBED]");
                return BT_GATT_ITER_STOP;
        }

	LOG_INF("[NOTIFICATION] data %d length %u", *((uint8_t *)data), length);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
                LOG_DBG("Discover complete");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

	if (!bt_uuid_cmp(disc_params.uuid, BT_UUID_DUMMY_SERVICE)) {
		memcpy(&uuid, BT_UUID_DUMMY_SERVICE_NOTIFY, sizeof(uuid));
		disc_params.uuid = &uuid.uuid;
		disc_params.start_handle = attr->handle + 1;
		disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &disc_params);
		if (err) {
			LOG_DBG("Discover failed (err %d)", err);
		}
	} else if (!bt_uuid_cmp(disc_params.uuid, BT_UUID_DUMMY_SERVICE_NOTIFY)) {
		disc_params.uuid = ccc_uuid;
		disc_params.start_handle = attr->handle + 2;
		disc_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

		value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &disc_params);
		if (err) {
			LOG_DBG("Discover failed (%d)", err);
		}
	} else {
		ccc_handle = attr->handle;

                SET_FLAG(gatt_discover_done_flag);

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
        int err;
        char addr_str[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

        LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

        err = bt_le_scan_stop();
        if (err) {
                FAIL("Failed to stop scanner (err %d)\n", err);
        }

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
        if (err) {
                FAIL("Could not connect to peer: %s (err %d)\n", addr_str, err);
        }
}

static void gatt_ccc_subscribe_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_subscribe_params *params)
{
        SET_FLAG(ccc_subscribed_flag);
        LOG_INF("[SUBSCRIBED]");
}

static void gatt_ccc_subscribe()
{
        int err;

	sub_params.notify = notify_cb;
	sub_params.subscribe = gatt_ccc_subscribe_cb;
	sub_params.value = BT_GATT_CCC_NOTIFY;
        sub_params.ccc_handle = ccc_handle;
        sub_params.value_handle = value_handle;

        err = bt_gatt_subscribe(default_conn, &sub_params);
	if (err && err != -EALREADY) {
		FAIL("Subscribe failed (err % d)\n ", err);
	}
}

static void gatt_ccc_unsubscribe()
{
        int err;

	sub_params.notify = notify_cb;
	sub_params.subscribe = NULL;
	sub_params.value = BT_GATT_CCC_NOTIFY;
	sub_params.ccc_handle = ccc_handle;
	sub_params.value_handle = value_handle;

	err = bt_gatt_unsubscribe(default_conn, &sub_params);
	if (err && err != -EALREADY) {
		if (err == -EINVAL) {
			LOG_INF("Unsubscribe failed no subscribtion found (err %d)", err);
		} else {
			FAIL("Unsubscribe failed (err %d)\n", err);
		}
	}
}

static void gatt_discover() {
        int err;

        memcpy(&uuid, BT_UUID_DUMMY_SERVICE, sizeof(uuid));
        disc_params.uuid = &uuid.uuid;
        disc_params.func = discover_func;
        disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
        disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
        disc_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(default_conn, &disc_params);
        if (err) {
                FAIL("Discover failed (err %d)\n", err);
        }

        WAIT_FOR_FLAG(gatt_discover_done_flag);
}

static bt_addr_le_t peer;

static void connected(struct bt_conn *conn, uint8_t err)
{
        const bt_addr_le_t *addr;
        char addr_str[BT_ADDR_LE_STR_LEN];

        addr = bt_conn_get_dst(conn);
        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

        if (err) {
                FAIL("Failed to connect to %s (err %d)\n", addr_str, err);
        }

        LOG_DBG("Connected: %s", addr_str);

	if (conn == default_conn) {
                if (bt_addr_le_cmp(BT_ADDR_LE_NONE, &peer) == 0) {
                        bt_addr_le_copy(&peer, addr);
                }
		SET_FLAG(connected_flag);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
        char addr_str[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

        LOG_DBG("Disconnected: %s (reason 0x%02x)", addr_str, reason);

        SET_FLAG(disconnected_flag);

        if (default_conn != conn) {
                return;
        }

        bt_conn_unref(default_conn);
        default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
        char addr_str[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

        if (!err) {
                LOG_DBG("Security changed: %s level %u", addr_str, level);
                SET_FLAG(security_updated_flag);
        } else {
                LOG_DBG("Security failed: %s level %u err %d", addr_str, level, err);
        }
}

static void start_scan(void)
{
        int err;

        err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
        if (err) {
                FAIL("Scanning failed to start (err %d)\n", err);
        }

        LOG_DBG("Scanning successfully started");
}

static void disconnect(void)
{
        int err;

        err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if (err) {
                FAIL("Failed to disconnect (err %d)\n", err);
	}

        WAIT_FOR_FLAG(disconnected_flag);
	UNSET_FLAG(disconnected_flag);
}

/* Test steps */

static void connect_pair_subscribe_get_notif()
{
        int err;

        start_scan();

        WAIT_FOR_FLAG(connected_flag);
	UNSET_FLAG(connected_flag);

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
        if (err != 0) {
                FAIL("Failed to set security (err %d)\n", err);
	}

	WAIT_FOR_FLAG(security_updated_flag);
	UNSET_FLAG(security_updated_flag);

	/* subscribe while being paired */
	gatt_discover();
        gatt_ccc_subscribe();

	WAIT_FOR_FLAG(ccc_subscribed_flag);
        UNSET_FLAG(ccc_subscribed_flag);

        WAIT_FOR_FLAG(notif_received_flag);
	UNSET_FLAG(notif_received_flag);
}

static void connect_unsubscribe()
{
	start_scan();

	WAIT_FOR_FLAG(connected_flag);
        UNSET_FLAG(connected_flag);

        LOG_DBG("Trying to unsubscribe whithout bonding...");

        gatt_ccc_unsubscribe();

	/* don't wait on `ccc_unsubscribed_flag` because it should _not_ be set */
	k_sleep(K_SECONDS(2));

        if (GET_FLAG(ccc_unsubscribed_flag)) {
                FAIL("CCC unsubscribed while not being bond\n");
        }
}

static void connect_set_sec_get_notif()
{
        int err;

	start_scan();

        WAIT_FOR_FLAG(connected_flag);
        UNSET_FLAG(connected_flag);

        err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
        if (err != 0) {
                FAIL("Failed to set security (err %d)\n", err);
        }

        WAIT_FOR_FLAG(security_updated_flag);
        UNSET_FLAG(security_updated_flag);

        LOG_INF("Waiting for notification...");

        WAIT_FOR_FLAG(notif_received_flag);
        UNSET_FLAG(notif_received_flag);

        gatt_ccc_unsubscribe();

        WAIT_FOR_FLAG(ccc_unsubscribed_flag);
        UNSET_FLAG(ccc_unsubscribed_flag);
}

void run_central(void)
{
        int err;

	bt_addr_le_copy(&peer, BT_ADDR_LE_NONE);

	central_cb.connected = connected;
	central_cb.disconnected = disconnected;
        central_cb.security_changed = security_changed;

	err = bt_enable(NULL);
	if (err) {
                FAIL("Bluetooth init failed (err %d)\n", err);
        }

        LOG_DBG("Bluetooth initialized");

	err = settings_load();
        if (err) {
                FAIL("Settings load failed (err %d)\n", err);
	}

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err) {
		FAIL("Unpairing failed (err %d)\n", err);
	}

	bt_conn_cb_register(&central_cb);

	connect_pair_subscribe_get_notif();
	disconnect();

        connect_unsubscribe();
        disconnect();

        connect_set_sec_get_notif();

        // we should also test that it's still possible to subscribe without being bonded

	PASS("Central test passed\n");
}
