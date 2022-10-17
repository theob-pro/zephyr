# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_link_libraries(tinycrypt)
target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC include)

zephyr_library_sources_ifdef(
  CONFIG_BT_H8
  h8.c
  )
