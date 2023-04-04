/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>

#include <zephyr/settings/settings.h>

#include "common.h"
#include "settings.h"

CREATE_FLAG(connected_flag);
CREATE_FLAG(disconnected_flag);
CREATE_FLAG(security_updated_flag);

CREATE_FLAG(ccc_notif_enabled_flag);

static struct bt_uuid_128 dummy_service = BT_UUID_INIT_128(DUMMY_SERVICE_TYPE);

static struct bt_uuid_128 notify_characteristic_uuid = BT_UUID_INIT_128(DUMMY_SERVICE_NOTIFY_TYPE);

static struct bt_conn *default_conn;

static struct bt_conn_cb peripheral_cb;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
        ARG_UNUSED(attr);

        bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

        LOG_INF("CCC Update: notification %s", notif_enabled ? "enabled" : "disabled");

        if (notif_enabled) {
                SET_FLAG(ccc_notif_enabled_flag);
        } else {
                UNSET_FLAG(ccc_notif_enabled_flag);
        }
}

BT_GATT_SERVICE_DEFINE(dummy_svc, BT_GATT_PRIMARY_SERVICE(&dummy_service),
		       BT_GATT_CHARACTERISTIC(&notify_characteristic_uuid.uuid, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void create_adv(struct bt_le_ext_adv **adv)
{
        int err;
        struct bt_le_adv_param params;

        memset(&params, 0, sizeof(struct bt_le_adv_param));

        params.options |= BT_LE_ADV_OPT_CONNECTABLE;

        params.id = BT_ID_DEFAULT;
        params.sid = 0;
        params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

        err = bt_le_ext_adv_create(&params, NULL, adv);
        if (err) {
                FAIL("Failed to create advertiser (%d)\n", err);
        }
}

static void start_adv(struct bt_le_ext_adv *adv)
{
        int err;
        int32_t timeout = 0;
        int8_t num_events = 0;

        struct bt_le_ext_adv_start_param start_params;

        start_params.timeout = timeout;
        start_params.num_events = num_events;

        err = bt_le_ext_adv_start(adv, &start_params);
        if (err) {
                FAIL("Failed to start advertiser (err %d)\n", err);
        }

        LOG_DBG("Advertiser started");
}

static void stop_adv(struct bt_le_ext_adv *adv)
{
        int err;

        err = bt_le_ext_adv_stop(adv);
        if (err) {
                FAIL("Failed to stop advertiser (err %d)\n", err);
        }
}

static void notify()
{
        struct bt_gatt_attr *notify_chrc = bt_gatt_find_by_uuid(dummy_svc.attrs, 0xffff, &notify_characteristic_uuid.uuid);

        uint8_t data = 0;

        LOG_INF("Start notify...");

        while (GET_FLAG(ccc_notif_enabled_flag)) {
                bt_gatt_notify(NULL, notify_chrc, &data, sizeof(data));
                data = (data + 1) % 255;
                k_msleep(500);
	}

        LOG_INF("Notify stopped.");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
        char addr_str[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

        if (err) {
                FAIL("Failed to connect to %s (err %d)\n", addr_str, err);
        }

	LOG_DBG("Connected: %s", addr_str);

        default_conn = bt_conn_ref(conn);

        SET_FLAG(connected_flag);
        UNSET_FLAG(disconnected_flag);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
        char addr_str[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

        LOG_DBG("Disconnected: %s (reason 0x%02x)", addr_str, reason);

        bt_conn_unref(conn);
        default_conn = NULL;

        SET_FLAG(disconnected_flag);
        UNSET_FLAG(connected_flag);
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

/* Test steps */

static void connect_pair_wait_subscribtion_notify(struct bt_le_ext_adv *adv)
{
        start_adv(adv);

        WAIT_FOR_FLAG(connected_flag);

	WAIT_FOR_FLAG(security_updated_flag);
        UNSET_FLAG(security_updated_flag);

	WAIT_FOR_FLAG(ccc_notif_enabled_flag);

        stop_adv(adv);

	notify(); /* notify while being paired, central should receive */
}

static void connect_wait_unsubscribtion(struct bt_le_ext_adv *adv)
{
        start_adv(adv);

        WAIT_FOR_FLAG(connected_flag);

        stop_adv(adv);

        /* Unsubscribe should NOT be triggered, peer did not re-encrypted link */
        k_sleep(K_SECONDS(2));

        if (GET_FLAG(ccc_notif_enabled_flag)) {
                FAIL("CCC Unsubscribed without required security level.\n");
        }
}

static void connect_pair_notify(struct bt_le_ext_adv *adv)
{
        start_adv(adv);

        WAIT_FOR_FLAG(connected_flag);

        WAIT_FOR_FLAG(security_updated_flag);
        UNSET_FLAG(security_updated_flag);

        WAIT_FOR_FLAG(ccc_notif_enabled_flag);

	notify(); /* notify while being paired, central should receive */
}

void run_peripheral(void)
{
        int err;
        struct bt_le_ext_adv *adv = NULL;

        peripheral_cb.connected = connected;
	peripheral_cb.disconnected = disconnected;
	peripheral_cb.security_changed = security_changed;

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

	bt_conn_cb_register(&peripheral_cb);

        create_adv(&adv);
        
        connect_pair_wait_subscribtion_notify(adv);
	WAIT_FOR_FLAG(disconnected_flag);

	connect_wait_unsubscribtion(adv);
	WAIT_FOR_FLAG(disconnected_flag);

	connect_pair_notify(adv);

        PASS("Peripheral test passed\n");
}
