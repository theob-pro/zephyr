/* Copyright (c) 2023 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <time.h>

#include <zephyr/init.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_btsnoop, LOG_LEVEL_DBG);

#include "btsnoop.h"

#define BTSNOOP_INIT_PRIORITY 60

static uint64_t btsnoop_ts_get(void)
{
        uint64_t cycles;

        if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
                cycles = k_cycle_get_64();
        } else {
                cycles = (uint64_t)k_cycle_get_32();
	}

	return k_cyc_to_us_floor64(cycles) + 0x00E03AB44A676000;
}

void bt_btsnoop_write(enum bt_btsnoop_flag flag, const void *data, size_t len)
{
	if (!IS_ENABLED(CONFIG_BT_BTSNOOP)) {
		return;
	}

        struct btsnoop_pkt_record pkt;

        pkt.original_len = sys_cpu_to_be32(len);
        pkt.included_len = sys_cpu_to_be32(len);
        pkt.flags = sys_cpu_to_be32(flag);
        pkt.drops = sys_cpu_to_be32(0);
        pkt.ts = sys_cpu_to_be64(btsnoop_ts_get());

	LOG_ERR("%s", __func__);
	LOG_HEXDUMP_DBG(data, len, "data:");

        btsnoop_write(pkt, data, len);
}

static void bt_btsnoop_write_hdr(void)
{
        struct btsnoop_hdr hdr;

        memcpy(hdr.id_pattern, "btsnoop", 8);
        hdr.version = sys_cpu_to_be32(1);
        hdr.datalink_type = sys_cpu_to_be32(1001); /* Un-encapsulated HCI (H1). The type of packet is indicated by the flag set. */

	btsnoop_write_hdr(hdr);
}

static int bt_btsnoop_init()
{
	if (IS_ENABLED(CONFIG_BT_BTSNOOP)) {
		btsnoop_init();
		bt_btsnoop_write_hdr();
        }

	return 0;
}

SYS_INIT(bt_btsnoop_init, PRE_KERNEL_1, BTSNOOP_INIT_PRIORITY);
