/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "zephyr/bluetooth/addr.h"
#include "zephyr/bluetooth/bluetooth.h"
#include "zephyr/bluetooth/conn.h"
#include "zephyr/toolchain/gcc.h"

#include <stdint.h>
#include <string.h>

BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");

void peripheral(void)
{
	bs_bt_utils_setup();

	printk("== Bonding id a - global bondable mode ==\n");
	advertise_connectable(BT_ID_DEFAULT, NULL);
	wait_connected();

	/* Central should bond here and trigger a disconnect. */
	wait_disconnected();

	clear_g_conn();

	PASS("PASS\n");
}
