# Copyright (C) 2025 EmbeddedSolutions.pl

idf_component_register(
  SRCS
    "ble_adv.c"
    "ble_gatt.c"
  INCLUDE_DIRS
    "include"
    "private"
  REQUIRES
  PRIV_REQUIRES
    bt
    em_scheduler
)
