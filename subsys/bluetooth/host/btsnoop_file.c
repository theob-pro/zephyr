/* Copyright (c) 2023 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/posix/fcntl.h>

#include "btsnoop.h"

#define BTSNOOP_FILENAME_LEN 12 + 7 + 1 /* 'log..btsnoop' + '{pid}' + '\n */

static int btsnoop_file_fd;

void btsnoop_open_file(void)
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

void btsnoop_file_write(const void *data, size_t len)
{
	if (write(btsnoop_file_fd, &data, len) != len) {
		close(btsnoop_file_fd);
                k_oops();
        }
}

void btsnoop_write(struct btsnoop_pkt_record pkt, const void *data, size_t len)
{
        btsnoop_file_write(&pkt, sizeof(pkt));
	btsnoop_file_write(data, len);
}

void btsnoop_write_hdr(struct btsnoop_hdr hdr)
{
        btsnoop_file_write(&hdr, sizeof(hdr));
}

void btsnoop_init(void)
{
        btsnoop_open_file();
}
