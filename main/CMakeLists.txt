# Copyright (C) 2024 EmbeddedSolutions.pl

# main component is automatically detected by the ESP-IDF Build-System
idf_component_register(
    SRCS
        "main.c"
)

add_subdirectory(modules)
