/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#include "bs_tracing.h"
#include "bstests.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bs_pc_backchannel.h"
#include "bs_types.h"
#include "argparse.h"

#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(backchannel_demo, LOG_LEVEL_DBG);

#define CHANNEL_ID_1 0 /* d0 <> d1 */
#define CHANNEL_ID_2 1 /* d0 <> d3 */
#define CHANNEL_ID_3 2 /* d1 <> d3 */

#define MSG_SIZE 1

int backchannel_init(void)
{
	uint device_number = bsim_args_get_global_device_nbr();
	uint device_numbers[2] = {0};
	uint channel_numbers[2] = {0};
	uint *ch;

	LOG_DBG("%s: device %d", __func__, device_number);

	if (device_number == 0) {
		device_numbers[0] = 1;
		device_numbers[1] = 2;

		channel_numbers[0] = CHANNEL_ID_1;
		channel_numbers[1] = CHANNEL_ID_2;
	} else if (device_number == 1) {
		device_numbers[0] = 0;
		device_numbers[1] = 2;

		channel_numbers[0] = CHANNEL_ID_1;
		channel_numbers[1] = CHANNEL_ID_3;
	} else if (device_number == 2) {
		device_numbers[0] = 0;
		device_numbers[1] = 1;

		channel_numbers[0] = CHANNEL_ID_2;
		channel_numbers[1] = CHANNEL_ID_3;
	} else {
		LOG_ERR("Device %d is unknown.", device_numbers);
		return -1;
	}

	ch = bs_open_back_channel(device_number, device_numbers, channel_numbers,
				  ARRAY_SIZE(channel_numbers));

	if (ch == NULL) {
		LOG_ERR("Backchannel initialization failed.");
		return -1;
	}

	LOG_DBG("Backchannel initialized.");

	return 0;
}

/**
 * @brief Wait for a sync message on channel_id
 *
 * @param channel_id this is the index of the channel ID `channel_numbers` in the `backchannel_init` function
 */
void backchannel_sync_wait(uint channel_id)
{
	uint8_t sync_msg[MSG_SIZE];
	uint device_number = bsim_args_get_global_device_nbr();

	LOG_DBG("Device %d is waiting for sync");

	while (true) {
		if (bs_bc_is_msg_received(channel_id) > 0) {
			bs_bc_receive_msg(channel_id, sync_msg, ARRAY_SIZE(sync_msg));
			if (sync_msg[0] != device_number) {
				/* Received a message from another device, exit */
				break;
			}
		}

		k_sleep(K_MSEC(1));
	}

	LOG_DBG("Sync received");
}

/**
 * @brief Send a sync message to a device listening on channel_id
 *
 * @param channel_id this is the index of the channel ID `channel_numbers` in the `backchannel_init` function
 */
void backchannel_sync_send(uint channel_id)
{
	uint device_number = bsim_args_get_global_device_nbr();
	uint8_t sync_msg[MSG_SIZE] = {device_number};

	LOG_DBG("Sync send");

	bs_bc_send_msg(channel_id, sync_msg, ARRAY_SIZE(sync_msg));
}

void entrypoint_device1(void)
{
	int err;

	err = backchannel_init();
	TEST_ASSERT(err == 0, "Failed to init backchannel");

	LOG_DBG("Waiting for device 1...");
	backchannel_sync_wait(0);

	LOG_DBG("Sending sync to device 3");
	backchannel_sync_send(1);

	TEST_PASS("Device 1 passed.");
}

void entrypoint_device2(void)
{
	int err;

	err = backchannel_init();
	TEST_ASSERT(err == 0, "Failed to init backchannel");

	LOG_DBG("Waiting for device 2...");
	backchannel_sync_wait(1);

	LOG_DBG("Sending sync to device 1");
	backchannel_sync_send(0);

	TEST_PASS("Device 2 passed.");
}

void entrypoint_device3(void)
{
	int err;

	err = backchannel_init();
	TEST_ASSERT(err == 0, "Failed to init backchannel");

	k_sleep(K_MSEC(1));

	LOG_DBG("Sending sync to device 1");
	backchannel_sync_send(1);

	LOG_DBG("Waiting for device 0");
	backchannel_sync_wait(0);

	TEST_PASS("Device 3 passed.");
}
