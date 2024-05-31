if(SB_CONFIG_HCI_IPC_NETCORE)
ExternalZephyrProject_Add(
  APPLICATION hci_ipc
  SOURCE_DIR ${ZEPHYR_BASE}/samples/bluetooth/hci_ipc
  BOARD ${SB_CONFIG_HCI_IPC_CORE_BOARD}
)
endif()
