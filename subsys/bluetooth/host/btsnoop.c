/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_btsnoop, LOG_LEVEL_DBG);

#include <zephyr/bluetooth/buf.h>

#include "btsnoop.h"

#define BTSNOOP_INIT_PRIORITY 60

#define BTSNOOP_FILENAME_LEN 12 + 7 + 1 /* 'log..btsnoop' + '{pid}' + '\n */

struct btsnoop_hdr {
        char id_pattern[8];
        uint32_t version;
        uint32_t datalink_type;
} __packed;

struct btsnoop_pkt_record {
        uint32_t original_len;
        uint32_t included_len;
        uint32_t flags;
        uint32_t drops;
        int64_t ts;
} __packed;

static int btsnoop_file_fd;
static const struct device *const btsnoop_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_snoop_uart));

static void btsnoop_open_file(void)
{
	pid_t pid = getpid(); // max value 2^22-1, 7 digits
	char filename[BTSNOOP_FILENAME_LEN]; // log.PID.btsnoop

	snprintk(filename, BTSNOOP_FILENAME_LEN, "log.%d.btsnoop", pid);

	btsnoop_file_fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, (mode_t)0666);
	if (btsnoop_file_fd == -1) {
		k_oops();
		return;
	}
}

static void btsnoop_uart_send(const void *data, size_t len)
{
	const uint8_t *buf = data;

	while (len--) {
		uart_poll_out(btsnoop_dev, *buf++);
        }
}

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

static void btsnoop_write(struct btsnoop_pkt_record pkt, const void *data, size_t len)
{
	IF_ENABLED(CONFIG_BT_BTSBOOP_FILE, ({
                if (write(btsnoop_file_fd, &pkt, sizeof(pkt)) == -1) {
                        close(btsnoop_file_fd);
                        return;
                }

                if (write(btsnoop_file_fd, data, len) == -1) {
                        close(btsnoop_file_fd);
                        return;
                }
        }))

	IF_ENABLED(CONFIG_BT_BTSNOOP_UART, ({
		btsnoop_uart_send(&pkt, sizeof(pkt));
		btsnoop_uart_send(data, len);
        }))
}

void bt_btsnoop_write(enum bt_btsnoop_flag flag, const void *data, size_t len)
{
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

static void btsnoop_write_hdr(void)
{
        struct btsnoop_hdr hdr;

        memcpy(hdr.id_pattern, "btsnoop", 8);
        hdr.version = sys_cpu_to_be32(1);
        hdr.datalink_type = sys_cpu_to_be32(1001); /* Un-encapsulated HCI (H1). The type of packet is indicated by the flag set. */

	IF_ENABLED(CONFIG_BT_BTSNOOP_FILE, ({
                if (write(btsnoop_file_fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
                        close(btsnoop_file_fd);
                        return;
                }
	}))

	IF_ENABLED(CONFIG_BT_BTSNOOP_UART, ({
                btsnoop_uart_send(&hdr, sizeof(hdr));
        }))
}

static void btsnoop_init(void)
{
	IF_ENABLED(CONFIG_BT_BTSNOOP_FILE, ({
		btsnoop_open_file();
        }))

	IF_ENABLED(CONFIG_BT_BTSNOOP_UART, ({
                __ASSERT_NO_MSG(device_is_ready(btsnoop_dev));

		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, ({
			uart_irq_rx_disable(btsnoop_dev);
			uart_irq_tx_disable(btsnoop_dev);
                }))
        }))

        btsnoop_write_hdr();
}

static int bt_btsnoop_init()
{
	IF_ENABLED(CONFIG_BT_BTSNOOP, ({
                btsnoop_init();
        }))

	return 0;
}

SYS_INIT(bt_btsnoop_init, PRE_KERNEL_1, BTSNOOP_INIT_PRIORITY);
