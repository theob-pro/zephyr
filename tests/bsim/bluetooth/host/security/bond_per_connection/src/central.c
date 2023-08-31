/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "zephyr/bluetooth/addr.h"
#include "zephyr/bluetooth/conn.h"

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");

extern void set_central(void);
void central(void)
{
	set_central();
	bs_bt_utils_setup();

	printk("== Bonding id a - global bondable mode ==\n");

	scan_connect_to_first_result();
	wait_connected();
	set_security(BT_SECURITY_L2);

	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);

	disconnect();
	wait_disconnected();
	
	clear_g_conn();

	PASS("PASS\n");
}
