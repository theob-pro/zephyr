#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh

# Define the executable name
bsim_exe=bs_nrf52_bsim_tests_bsim_bluetooth_host_security_bond_per_connection_prj_conf

# Build the project and copy the executable to the output directory
west build -b nrf52_bsim && \
    cp build/zephyr/zephyr.exe ${BSIM_OUT_PATH}/bin/${bsim_exe}
