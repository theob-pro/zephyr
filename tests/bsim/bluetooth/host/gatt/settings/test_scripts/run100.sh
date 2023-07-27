#!/usr/bin/env bash

log_file="result.log"
nbr_iter=999
nbr_failed_test=0

printf "000/%d" ${nbr_iter}
for i in $(seq 1 ${nbr_iter}); do
  printf "\r%03d/%d" ${i} ${nbr_iter}
  echo "=====================================================================" >> "${log_file}"
  echo "===============> iter ${i}" >> "${log_file}"
  echo "" >> "${log_file}"

  echo "------> run_gatt_settings" >> "${log_file}"
  echo "" >> "${log_file}"

  ./test_scripts/run_gatt_settings.sh >> "${log_file}" 2>&1
  if [ $? -ne 0 ]; then
    nbr_failed_test=$(( nbr_failed_test + 1 ))
    echo "run_gatt_settings failed at iter ${i}" >> "${log_file}"
    notify-send "Test failed (iter ${i})"
    # printf "\n"; exit 1
  fi

  echo "------> run_gatt_settings_2" >> "${log_file}"
  echo "" >> "${log_file}"

  ./test_scripts/run_gatt_settings_2.sh >> "${log_file}" 2>&1
  if [ $? -ne 0 ]; then
    nbr_failed_test=$(( nbr_failed_test + 1 ))
    echo "run_gatt_settings_2 failed at iter ${i}" >> "${log_file}"
    notify-send "Test failed (iter ${i})"
    # printf "\n"; exit 1
  fi

  echo "=====================================================================" >> "${log_file}"
done

printf "\r"

if [ ${nbr_failed_test} -eq 0 ]; then
  echo "All tests passed"
  echo "All tests passed" >> "${log_file}"
  notify-send "All tests passed"
else
  echo "${nbr_failed_test} tests failed"
  echo "${nbr_failed_test} tests failed" >> "${log_file}"
  notify-send "${nbr_failed_test} tests failed"
fi
