#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

BOARD="${BOARD:-nrf52_bsim}"
dut_exe="bs_${BOARD}_tests_bsim_bluetooth_host_deadlock_dut_prj_conf"
tester_exe="bs_${BOARD}_tests_bsim_bluetooth_host_deadlock_tester_prj_conf"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name="deadlock"
simulation_id="${test_name}"
verbosity_level=2
EXECUTE_TIMEOUT=30
sim_length_us=120e6

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=17 -sim_length=${sim_length_us} $@

for i in $(seq 1 16); do
    Execute "./$tester_exe" \
        -v=${verbosity_level} -s="${simulation_id}" -d=${i} -testid=tester -RealEncryption=1 -rs=$(( i * 10 ))
done

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut -RealEncryption=1

wait_for_background_jobs
