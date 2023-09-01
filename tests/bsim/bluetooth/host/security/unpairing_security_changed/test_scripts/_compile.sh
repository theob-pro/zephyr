#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh

set -eu
bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

source "${bash_source_dir}/_env.sh"

west build -b nrf52_bsim -d build && \
        cp -v build/zephyr/zephyr.exe "${test_exe}"
