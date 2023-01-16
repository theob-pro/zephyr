#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "smp.h"
#include "conn_internal.h"
#include "l2cap_internal.h"

static void start_scan(void);
static struct bt_conn *default_conn;

static void smp_send_bad()
{
	struct bt_smp_hdr *hdr;
	struct bt_smp_pairing_confirm *rsp;
	struct net_buf *buf;
	struct bt_smp *smp;

	buf = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
	__ASSERT_NO_MSG(buf);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_SMP_CMD_PAIRING_CONFIRM;

	printk("Inside smp_send_bad\n");

	if (bt_l2cap_send(default_conn, 0x0006, buf)) {
		
		net_buf_unref(buf);
	}

	printk("PDU sent\n");
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	if (bt_le_scan_stop()) {
        	// PASS("Peripheral test passed\n");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
		start_scan();
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	printk("Central Connected function\n");

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Connected: %s\n", addr);

	smp_send_bad();
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

static struct bt_conn_cb central_cb;

#if 0
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif

void test_central_main()
{
	printk("Sender\n");

	int err;

	central_cb.connected = connected;
        central_cb.disconnected = disconnected;

        bt_conn_cb_register(&central_cb);

	err = bt_enable(NULL);
	if (err) {
                printk("Bluetooth init failed (err %d)\n", err);
                return;
        }

	start_scan();

        // smp_send_bad();

	// printk("test\n");

        // PASS("Central test passed\n");
}
