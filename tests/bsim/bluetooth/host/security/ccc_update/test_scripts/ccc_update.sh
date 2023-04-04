#!/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

test_name='ccc_update'
test_exe="bs_${BOARD}_tests_bsim_bluetooth_host_security_${test_name}_prj_conf"
simulation_id="${test_name}"
verbosity_level=2
EXECUTE_TIMEOUT=30

cd ${BSIM_OUT_PATH}/bin

# Remove the files used by the custom SETTINGS backend
TO_DELETE="${simulation_id}_client.log ${simulation_id}_server.log"
echo "remove settings files ${TO_DELETE}"
rm ${TO_DELETE} || true

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=central -argstest "client"

Execute "./${test_exe}" \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=peripheral -argstest "server"

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=2 -sim_length=60e6 $@

wait_for_background_jobs
