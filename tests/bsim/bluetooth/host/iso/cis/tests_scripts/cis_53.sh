#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

BOARD=nrf5340bsim/nrf5340/cpuapp

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="$(guess_test_long_name)"
simulation_id=${test_name}
verbosity_level=2
EXECUTE_TIMEOUT=120

test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

echo "$test_exe"

cd ${BSIM_OUT_PATH}/bin


Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
    -D=2 -sim_length=30e6 $@
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central

wait_for_background_jobs
