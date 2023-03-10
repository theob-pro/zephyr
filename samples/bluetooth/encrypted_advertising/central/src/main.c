/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include <zephyr/bluetooth/ead.h>

#include "common.h"

extern int run_central_sample(uint8_t *received_data, struct material_key *keymat);

void main(void)
{
	(void)run_central_sample(NULL, NULL);
}
