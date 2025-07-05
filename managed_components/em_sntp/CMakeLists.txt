# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "sntp.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp_event
        esp_netif
        lwip
)
