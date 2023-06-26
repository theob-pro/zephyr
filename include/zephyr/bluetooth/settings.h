/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief This function act as a garbage collector for the Bluetooth settings.
 * It will:
 * - Remove not used settings key under 'bt/' namespace;
 * - Remove possible leftover from unexpected behavior.
 *
 * The settings are removed directly from the storage, the user need to run @ref
 * settings_load (or @ref settings_load_subtree to only load 'bt' namespace) to
 * apply the changes.
 *
 * Example of possible leftover: An identity has been deleted, the corresponding
 * address as been removed from 'bt/id' but before removing the corresponding
 * key in 'bt/keys', the device has been powered off. Calling
 * `bt_settings_cleanup` will ensure that the key is removed.
 *
 * @warning User must not call this function if they use the 'bt/' namespace to
 * store settings. All settings stored under 'bt/' namespace that are not
 * defined by the Bluetooth stack will be deleted.
 *
 * @note This function will ignore Bluetooth Mesh settings ('bt/mesh/'
 * namespace).
 *
 * @return 0 on success, non-zero on failure
 */
int bt_settings_cleanup(void);
