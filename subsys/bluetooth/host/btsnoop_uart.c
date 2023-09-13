/* Copyright (c) 2023 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/uart.h>

#include "btsnoop.h"

#define BTSNOOP_UART_SLIP_END     0xC0
#define BTSNOOP_UART_SLIP_ESC     0xDB
#define BTSNOOP_UART_SLIP_ESC_END 0xDC
#define BTSNOOP_UART_SLIP_ESC_ESC 0xDD

static const struct device *const btsnoop_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_snoop_uart));

void btsnoop_uart_slip_send_start(void)
{
	uart_poll_out(btsnoop_dev, BTSNOOP_UART_SLIP_END);
}

void btsnoop_uart_slip_send(const void *data, size_t len)
{
	const uint8_t *buf = data;

	while (len--) {
		switch (*buf) {
		case BTSNOOP_UART_SLIP_ESC:
			uart_poll_out(btsnoop_dev, BTSNOOP_UART_SLIP_ESC);
			uart_poll_out(btsnoop_dev, BTSNOOP_UART_SLIP_ESC_ESC);
			break;
		case BTSNOOP_UART_SLIP_END:
			uart_poll_out(btsnoop_dev, BTSNOOP_UART_SLIP_ESC);
			uart_poll_out(btsnoop_dev, BTSNOOP_UART_SLIP_ESC_END);
			break;
		default:
			uart_poll_out(btsnoop_dev, *buf);
		}

		buf++;
	}
}

void btsnoop_write(struct btsnoop_pkt_record pkt, const void *data, size_t len)
{
        btsnoop_uart_slip_send(&pkt, sizeof(pkt));
	btsnoop_uart_slip_send(data, len);
}

void btsnoop_write_hdr(struct btsnoop_hdr hdr)
{
        btsnoop_uart_slip_send_start();
        btsnoop_uart_slip_send(&hdr, sizeof(hdr));
}

void btsnoop_init_uart(void)
{
	__ASSERT_NO_MSG(device_is_ready(btsnoop_dev));

	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
                uart_irq_rx_disable(btsnoop_dev);
		uart_irq_tx_disable(btsnoop_dev);
        }
}

void btsnoop_init(void)
{
        btsnoop_init_uart();
}

