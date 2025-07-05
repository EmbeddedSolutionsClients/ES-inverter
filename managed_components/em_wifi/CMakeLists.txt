# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "src/wifi.c"
        "src/wifi_scan.c"
        "src/wifi_helpers.c"
        "src/wifi_apsta.c"
    INCLUDE_DIRS
        "include"
        "private"
    REQUIRES
        esp_event
    PRIV_REQUIRES
        esp_http_server
        esp_wifi
)
