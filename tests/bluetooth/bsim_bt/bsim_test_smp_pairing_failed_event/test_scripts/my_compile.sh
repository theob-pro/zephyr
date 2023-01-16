#!/usr/bin/env bash

west build -b nrf52_bsim
if [ $? -eq 0 ]; then
    cp -v build/zephyr/zephyr.exe ~/bsim/bin/bs_nrf52_bsim_tests_bluetooth_bsim_bt_bsim_test_smp_pairing_failed_event_prj_conf
fi