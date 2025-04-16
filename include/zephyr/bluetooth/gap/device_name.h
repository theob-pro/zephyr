/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_GAP_DEVICE_NAME_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_GAP_DEVICE_NAME_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Copy over Bluetooth GAP Device Name
 *
 * @note If buf is equal to NULL, the function will do nothing but return the
 * size of the current Bluetooth GAP Device Name.
 *
 * @param buf Buffer where the Bluetooth GAP Device Name will be copied
 * @param len Length of @p buf
 * @return size_t Size of the name or negative error TODO: improve me
 */
int bt_gap_get_device_name(uint8_t *buf, size_t len);

/**
 * @brief Set the Bluetooth GAP Device Name
 *
 * If the Bluetooth settings are enabled (CONFIG_BT_SETTINGS) and the storing
 * the name fail, no update will be made. The previous name will be kept.
 *
 * @note If the dynamic Bluetooth GAP Device Name is not enabled, the function
 * will do nothing an always return 0.
 *
 * @param name Buffer containing the new Bluetooth GAP device name
 * @return int 0 or negative error TODO: improve me
 */
int bt_gap_set_device_name(const uint8_t *name, size_t length);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_GAP_DEVICE_NAME_H_ */
