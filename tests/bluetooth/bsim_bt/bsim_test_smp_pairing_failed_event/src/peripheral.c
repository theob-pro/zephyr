#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

static struct bt_conn *default_conn;

static const struct bt_data adv_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))};


static void connected(struct bt_conn *conn, uint8_t err)
{
	printk("Peripheral Connected function\n");

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Connected: %s\n", addr);

        default_conn = bt_conn_ref(conn);
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
}

static struct bt_conn_cb peri_cb;
#if 0
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif


void test_peripheral_main()
{
	printk("Receiver\n");

        int err;

        peri_cb.connected = connected;
        peri_cb.disconnected = disconnected;

        bt_conn_cb_register(&peri_cb);

        err = bt_enable(NULL);
        if (err) {
                printk("Bluetooth init failed (err %d)\n", err);
                return;
        }

        printk("Bluetooth initialized\n");

	bt_le_adv_start(BT_LE_ADV_CONN_NAME, adv_ad_data, ARRAY_SIZE(adv_ad_data), NULL, 0);

        while (true) {
	        k_sleep(K_SECONDS(1));
        }

        // PASS("Peripheral test passed\n");
}
