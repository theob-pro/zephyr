/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/net/buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "common/bt_str.h"

#include "util/gatt_discover.h"
#include "util/att_read.h"
#include "util/signal.h"
#include "util/scan.h"

#include "common.h"

LOG_MODULE_REGISTER(ead_central_sample, CONFIG_BT_EAD_LOG_LEVEL);

DEFINE_FLAG(wait_security_update);
DEFINE_FLAG(wait_pairing_confirm);
DEFINE_FLAG(wait_data_received);

static struct material_key keymat;
static uint8_t *received_data;

static bt_addr_le_t peer_addr;
static struct bt_conn *default_conn;

static struct bt_conn_cb central_cb;
static struct bt_conn_auth_cb central_auth_cb;

static bool data_parse_cb(struct bt_data *data, void *user_data)
{
	if (data->type == BT_DATA_ENCRYPTED_AD_DATA) {
		int err;
		struct net_buf_simple decrypted_buf;
		size_t decrypted_data_size = BT_EAD_DECRYPTED_PAYLOAD_SIZE(data->data_len);
		uint8_t decrypted_data[decrypted_data_size];

		net_buf_simple_init_with_data(&decrypted_buf, &decrypted_data, decrypted_data_size);

		err = bt_ead_decrypt(keymat.session_key, keymat.iv, data->data, data->data_len,
				     decrypted_buf.data);
		if (err < 0) {
			LOG_ERR("Error during decryption.");
		}

		bt_data_parse(&decrypted_buf, &data_parse_cb, user_data);
	} else {
		LOG_INF("len : %d", data->data_len);
		LOG_INF("type: 0x%02x", data->type);
		LOG_HEXDUMP_INF(data->data, data->data_len, "data:");

		if (received_data != NULL) {
			size_t offset = *((size_t *)user_data);

			offset += bt_data_serialize(data, &received_data[offset]);

			*((size_t *)user_data) = offset;
		} else {
			*((size_t *)user_data) += bt_data_get_len(data, 1);
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	size_t parsed_data_size;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	/* We are only interested in the previously connected device. */
	if (!bt_addr_le_eq(addr, &peer_addr)) {
		return;
	}

	LOG_DBG("Peer found.");

	parsed_data_size = 0;
	LOG_INF("Received data size: %zu", ad->len);
	bt_data_parse(ad, data_parse_cb, &parsed_data_size);

	LOG_DBG("All data parsed. (total size: %zu)", parsed_data_size);
	SET_FLAG(wait_data_received);

	if (bt_le_scan_stop()) {
		return;
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		LOG_DBG("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_DBG("Scanning started.");
}

struct k_poll_signal conn_signal;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_DBG("Failed to connect to %s (err %u)", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();

		return;
	}

	LOG_DBG("Connected: %s %p %p", addr, (void *)conn, (void *)default_conn);

	k_poll_signal_raise(&conn_signal, 0);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr, level);
		SET_FLAG(wait_security_update);
	} else {
		LOG_DBG("Security failed: %s level %u err %d", addr, level, err);
	}
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	LOG_DBG("Identity resolved %s -> %s", addr_rpa, addr_identity);

	bt_addr_le_copy(&peer_addr, identity);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	LOG_DBG("Confirm passkey for %s: %s", addr, passkey_str);

	SET_FLAG(wait_pairing_confirm);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	LOG_DBG("Passkey for %s: %s", addr, passkey_str);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Pairing cancelled: %s", addr);
}

int run_central_sample(uint8_t *test_received_data, struct material_key *test_received_keymat)
{
	int err;
	uint16_t end_handle;
	uint16_t start_handle;
	struct bt_util_att_read read_result;
	struct bt_util_att_read read_result2;
	struct bt_util_scan_find_name scan_result;
	struct gatt_service_discovery disc_result;

	if (test_received_data != NULL) {
		received_data = test_received_data;
	}

	/* Initialize Bluetooth and callbacks. */

	{
		central_cb.connected = connected;
		central_cb.disconnected = disconnected;
		central_cb.security_changed = security_changed;
		central_cb.identity_resolved = identity_resolved;

		bt_conn_cb_register(&central_cb);

		default_conn = NULL;

		err = bt_enable(NULL);
		if (err) {
			LOG_ERR("Bluetooth init failed (err %d)", err);
			return -1;
		}

		LOG_DBG("Bluetooth initialized");

		central_auth_cb.pairing_confirm = NULL;
		central_auth_cb.passkey_confirm = auth_passkey_confirm;
		central_auth_cb.passkey_display = auth_passkey_display;
		central_auth_cb.passkey_entry = NULL;
		central_auth_cb.oob_data_request = NULL;
		central_auth_cb.cancel = auth_cancel;

		err = bt_conn_auth_cb_register(&central_auth_cb);
		if (err != 0) {
			return -1;
		}
	}

	/* Find and connect to our peripheral. */

	{
		LOG_DBG("Start scan");

		bt_util_scan_find_name(&scan_result, "EAD Sample");
		if (scan_result.api_err) {
			LOG_DBG("Could not start scanner. %d", scan_result.api_err);
			return -1;
		}

		LOG_DBG("Device found. Connecting %s", bt_addr_le_str(&scan_result.addr));
		k_poll_signal_init(&conn_signal);

		bt_conn_le_create(&scan_result.addr, BT_CONN_LE_CREATE_CONN,
				  BT_LE_CONN_PARAM_DEFAULT, &default_conn);
		if (err != 0) {
			LOG_DBG("Failed to connect to %s", bt_addr_le_str(&scan_result.addr));
			return -1;
		}

		LOG_DBG("Wait connection");
		bt_util_await_signal(&conn_signal);
	}

	/* Update connection security level. */

	{
		err = bt_conn_set_security(default_conn, BT_SECURITY_L4);
		if (err != 0) {
			LOG_ERR("Failed to set security (err %d)", err);
			return -1;
		}

		WAIT_FOR_FLAG(wait_pairing_confirm);

		bt_conn_auth_passkey_confirm(default_conn);
		if (err != 0) {
			LOG_DBG("Failed to confirm passkey.");
			return -1;
		}

		WAIT_FOR_FLAG(wait_security_update);
	}

	/* Locate the primary service. */

	{
		LOG_DBG("Starting primary service discovery...");

		bt_util_gatt_discover_primary_service_sync(&disc_result, default_conn,
							   BT_UUID_CUSTOM_SERVICE);
		if (disc_result.api_err || disc_result.att_err) {
			LOG_DBG("GATT Service not found (bad stuff).");
			return -1;
		}

		start_handle = disc_result.start_handle;
		end_handle = disc_result.end_handle;

		LOG_DBG("Discovery done.");
	}

	/* Read the Material Key characteristic. */

	{
		read_result.read_len = sizeof(keymat);
		read_result.read_buf = (uint8_t *)&keymat;

		bt_util_att_read_by_type_sync(&read_result, default_conn,
					      BT_ATT_CHAN_OPT_UNENHANCED_ONLY, BT_UUID_GATT_EDKM,
					      start_handle, end_handle);
		if (read_result.api_err != 0 || read_result.att_err != 0) {
			LOG_DBG("ATT Read fail.");
			return -1;
		}

		LOG_DBG("Done %d %u", read_result.api_err, read_result.att_err);

		/* The key material will not fit in a single PDU with the default MTU. But it will
		 * fit the rest in the following.
		 */
		if (read_result.read_len < sizeof(keymat)) {
			read_result2.read_len = sizeof(keymat) - read_result.read_len;
			read_result2.read_buf = &read_result.read_buf[read_result.read_len];

			bt_util_att_read_by_handle(&read_result2, default_conn,
						   BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
						   read_result.handle, read_result.read_len);
			if (read_result.api_err != 0 || read_result.att_err != 0) {
				LOG_DBG("ATT Read fail.");
				return -1;
			}

			LOG_DBG("Done %d %u", read_result.api_err, read_result.att_err);
		}

		LOG_HEXDUMP_DBG(keymat.session_key, BT_EAD_KEY_SIZE, "Session Key");
		LOG_HEXDUMP_DBG(keymat.iv, BT_EAD_IV_SIZE, "IV");
	}

	if (test_received_keymat != NULL) {
		memcpy(test_received_keymat, &keymat, sizeof(keymat));
	}

	/* Start a new scan to get the Advertising Data. */

	{
		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			LOG_DBG("Failed to disconnect.");
			return -1;
		}

		start_scan();
	}

	WAIT_FOR_FLAG(wait_data_received);

	return 0;
}
