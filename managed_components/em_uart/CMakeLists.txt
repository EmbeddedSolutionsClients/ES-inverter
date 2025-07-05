# Copyright (C) 2025 EmbeddedSolutions.pl

idf_component_register(
    SRCS
        "uart.c"
        "uart_rx.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_driver_uart
)
