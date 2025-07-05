# Copyright (C) 2025 EmbeddedSolutions.pl

idf_component_register(
  SRCS
   "ble_prov_svc.c"
  INCLUDE_DIRS
    "include"
    "private"
  REQUIRES
  PRIV_REQUIRES
    em_ble_core
    em_wifi
		em_scheduler
    bt
)
