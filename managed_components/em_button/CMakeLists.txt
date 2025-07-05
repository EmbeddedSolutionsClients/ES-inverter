# Copyright (C) 2024 EmbeddedSolutions.pl

idf_component_register(
    SRCS
       "button.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        espressif__button
        esp_event
)
